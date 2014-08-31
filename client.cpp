#include <boost/program_options.hpp>

// using namespace boost;
namespace po = boost::program_options;

#include <iostream>
#include <iterator>

#include <regex>

#include <pthread.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using boost::asio::ip::tcp;

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

boost::asio::io_service ioService;

boost::asio::deadline_timer connection_timer(ioService, boost::posix_time::seconds(1));

boost::asio::deadline_timer keepalive_timer(ioService, boost::posix_time::milliseconds(100));

bool connectionIsProblematic = false;

bool connectionIsAlive = true;

void setup(int ac, char* av[])
{
    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("port,p", po::value<long>(&PORT)
                  ->default_value(10000 + 334678 % 10000,"no"),
                  "listen on a port.")
            ("retransmit-limit,X", po::value<long>(&RETRANSMIT_LIMIT)
                  ->default_value(10,"no"),
                  "")
            ("server,s", po::value<string>(),
                  "server")
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
            exit(0);
        }

        if (vm.count("port"))
        {
            PORT = vm["port"].as<long>();
            cout << "port: "
                 << PORT << "\n";
        }

        if (vm.count("server"))
        {
            SERVER = vm["server"].as<string>();
            cout << "server: "
                 << SERVER << "\n";
        }

        if (vm.count("retransmit-limit"))
        {
            RETRANSMIT_LIMIT = vm["retransmit-limit"].as<long>();
            cout << "RETRANSMIT_LIMIT: "
                 << RETRANSMIT_LIMIT << "\n";
        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        exit(1);
    }
}

void keepalive(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    // cerr << "keepalive\n";
    t->expires_at(t->expires_at() + boost::posix_time::milliseconds(100));
    t->async_wait(boost::bind(keepalive,
            boost::asio::placeholders::error, t));
}

/**
 * Zadanie odpowiedzialne za okresowe przesyłanie komunikatu keepalive
 */
void keepalive_task()
{
    // cerr << "keepalive_task\n";

    keepalive_timer.async_wait(boost::bind(keepalive,
            boost::asio::placeholders::error, &keepalive_timer));
}

void check_connection(const boost::system::error_code& /*e*/, boost::asio::deadline_timer* t)
{
    // cerr << "check_connection\n";
    // if (!connectionIsAlive)
    // {
    //     connectionIsProblematic = true;
        t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
        t->async_wait(boost::bind(check_connection,
                boost::asio::placeholders::error, t));
    // }
    // else
    //     connectionIsAlive = false;
}

/**
 * Zadanie odpowiedzialne za sprawdzanie ciągłość przesyłu danych od serwera
 */
void check_connection_task()
{
    // cerr << "check_connection_task\n";

    connection_timer.async_wait(boost::bind(check_connection,
            boost::asio::placeholders::error, &connection_timer));
}

void connect_tcp()
{
    cout << "connect_tcp\n";
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(SERVER, to_string(PORT));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    tcp::socket tcp_socket(ioService);
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
        boost:array<char, 256> buf;
        boost::system::error_code error;

        cout << "Reading\n";

        size_t len = tcp_socket.read_some(boost::asio::buffer(buf), error);

        cout << "Done\n";

        if (error == boost::asio::error::eof)
        {
            cout << "1\n";
            // break; // TODO Connection closed cleanly by peer
        }
        else if (error)
        {
            cout << "2\n";
            throw boost::system::system_error(error); // Some other error.
        }

        std::cout.write(buf.data(), len);
    }
    catch (exception& e)
    {

    }

}

void identifyMessage(string message)
{
    std::regex regex("^DATA");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // DATA nr ack win\n
        // [dane]

        // Wysyłany przez serwer datagram z danymi.
        // Nr to numer datagramu, zwiększany o jeden przy każdym kolejnym fragmencie danych;
        // służy do identyfikacji datagramów na potrzeby retransmisji.
        // Ack to numer kolejnego datagramu (patrz UPLOAD) oczekiwanego od klienta,
        // a win to liczba wolnych bajtów w FIFO.

        // Dane należy przesyłać w formie binarnej, dokładnie tak, jak wyprodukował je mikser.
        return;
    }

    regex = std::regex("^ACK");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // ACK ack win\n
        // [dane]

        // Wysyłany przez serwer datagram potwierdzający otrzymanie danych.
        // Jest wysyłany po odebraniu każdego poprawnego datagramu UPLOAD.
        // Znaczenie ack i win -- patrz DATA.
        return;
    }
}

int main(int ac, char* av[])
{
    cerr << "Client reporting\n" << endl;

    setup(ac, av);
    keepalive_task();
    check_connection_task();
    connect_tcp();

    //Przyjmuj dane ze standardowego wejścia i wrzucaj na serwer

    //zakończ przyjmowanie

    //Odbieraj dane od serwera aż do sigint
    ioService.run();

    return 0;
}