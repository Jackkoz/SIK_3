#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <cstdlib>

#include <regex>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using namespace boost::posix_time;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

long PORT;

long RETRANSMIT_LIMIT;

string SERVER;

long myID = -1;

boost::asio::io_service ioService;

boost::asio::posix::stream_descriptor in(ioService, ::dup(STDIN_FILENO));

tcp::socket tcp_socket(ioService);

boost::array<char, 16384> tcp_buffer;

udp::socket udp_socket(ioService);

char udp_receive_buffer[65536];

char udp_send_buffer[65536];

char udp_datagram[65536];

long win = 0;

udp::endpoint udp_receiver_endpoint;

boost::asio::deadline_timer connection_timer(ioService); //, boost::posix_time::seconds(1));

boost::asio::deadline_timer keepalive_timer(ioService); // , boost::posix_time::milliseconds(100));

boost::asio::deadline_timer stdin_timer(ioService); //, boost::posix_time::milliseconds(10));

bool connectionIsAlive = true;

ptime lastRestart = microsec_clock::local_time();

// Forward declarations

bool setup(int ac, char* av[]);
void udp_write_handler(const boost::system::error_code&);
void keepalive(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t);
void keepalive_task();
void check_connection(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t);
void check_connection_task();
void tcp_receive_reports();
void tcp_handler(const boost::system::error_code &error, size_t bytes_transferred);
void tcp_receive_reports();
void connect_tcp();
void connect_udp();
void udp_communicate();
void handle_data_datagram();
void handle_ack_datagram();
void udp_communication_handler(const boost::system::error_code &error, size_t bytes_transferred);
void reboot_client();
void start_server();

/**
 * Ustawienie zmiennych serwera na domyślne lub przeczytane z linii poleceń.
 */
bool setup(int ac, char* av[])
{
    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("port,p", po::value<long>(&PORT)
                  ->default_value(10000 + 334678 % 10000),
                  "Connection port")
            ("retransmit-limit,X", po::value<long>(&RETRANSMIT_LIMIT)
                  ->default_value(10),
                  "Limit of retransitions")
            ("server,s", po::value<string>(&SERVER),
                  "Remote server address")
        ;

        po::positional_options_description p;
        p.add("input-file", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(ac, av).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << "Usage: options_description [options]\n";
            cout << desc;
            return false;
        }

        if (! vm.count("server"))
        {
            cout << "Please provide the server address.\n";
            return false;
        }

        cerr << "Server configuration:\n";
        cerr << "SERVER " << SERVER;
        cerr << "\nPORT: " << PORT;
        cerr << "\nRETRANSMIT_LIMIT " << RETRANSMIT_LIMIT << '\n';

        return true;
    }
    catch(std::exception& e)
    {
        cerr << e.what() << '\n';
        return false;
    }
}

void udp_write_handler(const boost::system::error_code&) {}

/**
 * Zadanie odpowiedzialne za okresowe przesyłanie komunikatu keepalive
 */
void keepalive(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    char buffer[16];
    sprintf(buffer, "keepalive");

    udp_socket.async_send_to(boost::asio::buffer(buffer), udp_receiver_endpoint,
                             boost::bind(&udp_write_handler,
                                    boost::asio::placeholders::error));

    t->expires_at(t->expires_at() + boost::posix_time::milliseconds(100));
    t->async_wait(boost::bind(&keepalive,
            boost::asio::placeholders::error, t));
}

void keepalive_task()
{
    keepalive_timer.async_wait(boost::bind(&keepalive,
            boost::asio::placeholders::error, &keepalive_timer));
}

/**
 * Zadanie odpowiedzialne za sprawdzanie ciągłości przesyłu danych od serwera
 */
void check_connection(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    if (connectionIsAlive)
    {
        connectionIsAlive = false;
        t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
        t->async_wait(boost::bind(&check_connection,
                boost::asio::placeholders::error, t));
    }
    else
    {
        cerr << "Connection determined dead\n";
        reboot_client();
    }
}

void check_connection_task()
{
    connection_timer.async_wait(boost::bind(check_connection,
            boost::asio::placeholders::error, &connection_timer));
}

/**
 * Obsługa przeczytanego reportu
 */
void tcp_handler(const boost::system::error_code &error, size_t bytes_transferred)
{
    if (error)
    {
        cerr << "An error occurred when reading TCP: " << error.message() << "\n";
        reboot_client();
        return;
    }

    cerr.write(tcp_buffer.data(), bytes_transferred);

    tcp_receive_reports();
}

/**
 * Asynchroniczne oczekiwanie na report serwera
 */
void tcp_receive_reports()
{
    tcp_socket.async_read_some(boost::asio::buffer(tcp_buffer),
            boost::bind(&tcp_handler,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

/**
 * Nawiązanie połączenia TCP z serwerem
 */
void connect_tcp()
{
    cerr << "Connecting on TCP\n";
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(SERVER, to_string(PORT));
    tcp::resolver::iterator endpoint_iterator;
    cerr << "Resolving TCP query\n";
    try
    {
        endpoint_iterator = resolver.resolve(query);
    }
    catch (exception& e)
    {
        cerr << "Error resolving TCP Query\n";
        reboot_client();
    }
    tcp::resolver::iterator end;

    cerr << "Attempting TCP connection\n";

    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        tcp_socket.close();
        tcp_socket.connect(*endpoint_iterator++, error);
    }

    if (error)
    {
        cerr << "Error on creating TCP connection: " << error.message() << '\n';

        if (error.value() == boost::asio::error::invalid_argument)
        {
            cerr << "Shutting down server\n";
            exit(0);
        }

        reboot_client();
        return;
    }

    try
    {
        char buf[256];
        boost::system::error_code error;

        cerr << "Reading TCP response\n";

        size_t len = tcp_socket.read_some(boost::asio::buffer(buf), error);

        if (error)
        {
            cerr << "Error on reading TCP connection: " << error.message() << '\n';
            reboot_client();
            return;
        }

        // cerr << buf << '\n';

        cerr << "Reading ID\n";

        sscanf(buf, "CLIENT %ld\n", &myID);

        cout << "Obtained id: " + myID + '\n';
    }
    catch (exception& e)
    {
        cerr << "Error on creating TCP connection: " << e.what() << '\n';
        reboot_client();
        return;
    }
}

/**
 * Nawiązanie połączenia UDP z serwerem
 */
void connect_udp()
{
    udp::resolver resolver(ioService);
    udp::resolver::query query(SERVER, to_string(PORT));

    udp_receiver_endpoint = *resolver.resolve(query);

    try
    {
        udp_socket.open(udp::v6());
    }
    catch (exception& e)
    {
        cerr << "Error on creating UDP connection: " << e.what() << '\n';
        reboot_client();
        return;
    }

    char buffer[256];

    sprintf(buffer, "CLIENT %ld\n", myID);

    udp_socket.async_send_to(boost::asio::buffer(buffer), udp_receiver_endpoint,
                             boost::bind(udp_write_handler,
                                    boost::asio::placeholders::error));
}

void handle_data_datagram()
{

}

void handle_ack_datagram()
{

}

void udp_communication_handler(const boost::system::error_code &error, size_t bytes_transferred)
{
    connectionIsAlive = true;

   if (error)
    {
        cerr << "An error occurred while reading UDP: " << error.message() << "\n";
        reboot_client();
        return;
    }

    cout << "Reading response header\n";

    char datagram_type[16];

    sscanf(udp_receive_buffer, "%s ", datagram_type);

    cout << datagram_type << '\n';

    if (strcmp("DATA", datagram_type) == 0)
        handle_data_datagram();
    else if (strcmp("ACK", datagram_type) == 0)
        handle_ack_datagram();
    else
        cerr << "Unknown datagram, ignoring\n";

    cout << "Handled\n";

    // cout.write(udp_receive_buffer, bytes_transferred);
    udp_communicate();
}

void udp_communicate()
{
    udp_socket.async_receive_from(
            boost::asio::buffer(udp_receive_buffer), udp_receiver_endpoint,
            boost::bind(&udp_communication_handler,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
}

void reboot_client()
{
    cerr << "Rebooting client\n";

    try
    {
        tcp_socket.close();
        cerr << "TCP socket closed\n";
    }
    catch (exception& e) {}

    try
    {
        udp_socket.close();
        cerr << "UDP socket closed\n";
    }
    catch (exception& e)
    {
        cerr << e.what() << '\n';
    }

    keepalive_timer.cancel();
    connection_timer.cancel();
    cerr << "Timers cancelled\n";

    time_duration timeSinceRestart = microsec_clock::local_time() - lastRestart;
    long milliseconds = timeSinceRestart.total_milliseconds();

    if (milliseconds < 500)
    {
        cerr << "Sleeping for: " << 500 - milliseconds << " milliseconds\n";
        usleep((500 - milliseconds) * 1000);
        cerr << "Awake\n";
    }

    lastRestart = microsec_clock::local_time();

    start_server();

    cerr << "Client rebooted\n";
}

void start_server()
{
    tcp_socket = tcp::socket(ioService);
    cerr << "Created TCP socket\n";

    udp_socket = udp::socket(ioService);
    cerr << "Created UDP socket\n";

    connectionIsAlive = true;


    connect_tcp();
    cerr << "Connected on TCP\n";

    connect_udp();
    cerr << "Connected on UDP\n";

    udp_communicate();
    cerr << "Started UDP communication\n";

    keepalive_task();
    cerr << "Keepalive task started\n";

    tcp_receive_reports();
    cerr << "Reports receiving task started\n";

    check_connection_task();
    cerr << "Connection checking task started\n";
}

int main(int ac, char* av[])
{
    if (!setup(ac, av))
    {
        cerr << "Invalid client setup\nShutting down\n";
        exit(0);
    }

    start_server();

    ioService.run();
    return 0;
}