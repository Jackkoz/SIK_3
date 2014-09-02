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
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

// A helper function to simplify program options
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

long PORT;

long RETRANSMIT_LIMIT;

string SERVER;

long myID = 1;

boost::asio::io_service ioService;

tcp::socket tcp_socket(ioService);

boost::array<char, 16384> tcp_buffer;

udp::socket udp_socket(ioService);

char udp_receive_buffer[16384];

char udp_send_buffer[16384];

char udp_datagram[16384];

long win;

udp::resolver resolver(ioService);

udp::resolver::query query(SERVER, to_string(PORT));

udp::endpoint udp_receiver_endpoint = *resolver.resolve(query);

boost::asio::deadline_timer connection_timer(ioService, boost::posix_time::seconds(1));

boost::asio::deadline_timer keepalive_timer(ioService, boost::posix_time::milliseconds(100));

bool connectionIsProblematic = false;

bool connectionIsAlive = true;

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

        if (vm.count("port"))
        {
            PORT = vm["port"].as<long>();
        }

        if (vm.count("server"))
        {
            SERVER = vm["server"].as<string>();
        }
        else
        {
            cout << "Please provide the server address.\n";
            return false;
        }

        if (vm.count("retransmit-limit"))
        {
            RETRANSMIT_LIMIT = vm["retransmit-limit"].as<long>();
        }

        cerr << "PORT: " << PORT;
        cerr << "\nSERVER " << SERVER;
        cerr << "\nRETRANSMIT_LIMIT " << RETRANSMIT_LIMIT << '\n';

        return true;
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return false;
    }
}

void keepalive(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    t->expires_at(t->expires_at() + boost::posix_time::milliseconds(100));
    t->async_wait(boost::bind(&keepalive,
            boost::asio::placeholders::error, t));
}

void udp_write_handler(const boost::system::error_code&) {}

/**
 * Zadanie odpowiedzialne za okresowe przesyłanie komunikatu keepalive
 */
void keepalive_task()
{
    char buffer[256];
    sprintf(buffer, "keepalive");

    udp_socket.async_send_to(boost::asio::buffer(buffer), udp_receiver_endpoint,
                             boost::bind(&udp_write_handler,
                                    boost::asio::placeholders::error));

    keepalive_timer.async_wait(boost::bind(&keepalive,
            boost::asio::placeholders::error, &keepalive_timer));
}

void check_connection(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    if (!connectionIsAlive)
    {
        connectionIsProblematic = true;
        t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
        t->async_wait(boost::bind(&check_connection,
                boost::asio::placeholders::error, t));
    }
    else
        connectionIsAlive = false;
}

/**
 * Zadanie odpowiedzialne za sprawdzanie ciągłości przesyłu danych od serwera
 */
void check_connection_task()
{
    connection_timer.async_wait(boost::bind(check_connection,
            boost::asio::placeholders::error, &connection_timer));
}

/**
 * Forward declaration for tcp_handler
 */
void tcp_receive_raports();

/**
 * Obsługa przeczytanego raportu
 */
void tcp_handler(const boost::system::error_code &error, size_t bytes_transferred)
{
    if (error)
    {
        cerr << "An error occurred: " << error.message() << "\n";
        connectionIsProblematic = true;
    }

    // cout.write(tcp_buffer.data(), bytes_transferred);

    tcp_receive_raports();
}

/**
 * Asynchroniczne oczekiwanie na raport serwera
 */
void tcp_receive_raports()
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
    cout << "connect_tcp\n";
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(SERVER, to_string(PORT));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        tcp_socket.close();
        tcp_socket.connect(*endpoint_iterator++, error);
    }

    if (error)
        throw boost::system::system_error(error);

    try
    {
        char buf[256];
        boost::system::error_code error;

        size_t len = tcp_socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof)
        {
            connectionIsProblematic = true;
        }
        else if (error)
        {
            connectionIsProblematic = true;
            // throw boost::system::system_error(error); // Some other error.
        }

        sscanf(buf, "CLIENT %d\n", &myID);

        // cout << myID;
    }
    catch (exception& e)
    {
        // HANDLE
        connectionIsProblematic = true;
    }
}

void connect_udp()
{
    udp_socket.open(udp::v6());

    char buffer[256];

    sprintf(buffer, "CLIENT %d\n", myID);

    udp_socket.async_send_to(boost::asio::buffer(buffer), udp_receiver_endpoint,
                             boost::bind(udp_write_handler,
                                    boost::asio::placeholders::error));
}

/**
 * Forward declaration for udp_communication_handler()
 */
void udp_communicate();

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
        cerr << "An error occurred: " << error.message() << "\n";
        connectionIsProblematic = true;
    }

    char datagram_type[16];

    sscanf(udp_receive_buffer, "%s ", datagram_type);

    // handle datagram

    if (strcmp("DATA", datagram_type) == 0)
        handle_data_datagram();
    else if (strcmp("ACK", datagram_type) == 0)
        handle_ack_datagram();
    // else unknown datagram; ignore

    // cout.write(tcp_buffer.data(), bytes_transferred);
    udp_communicate();
}

void udp_communicate()
{
    udp_socket.async_receive_from(
                            boost::asio::buffer(udp_receive_buffer), udp_receiver_endpoint,
                            boost::bind(udp_communication_handler,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void reboot()
{
    // TODO

    // Free resources

    // Re-run procedures

    connectionIsProblematic = false;
}

int main(int ac, char* av[])
{
    if (!setup(ac, av))
        exit(0);

    connect_tcp();
    keepalive_task();
    tcp_receive_raports();
    check_connection_task();

    ioService.run();
    return 0;
}