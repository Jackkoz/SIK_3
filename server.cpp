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

class Client;

// PORT - numer portu, z którego korzysta serwer do komunikacji
// (zarówno TCP, jak i UDP), domyślnie 10000 + (numer_albumu % 10000);
// ustawiany parametrem -p serwera, opcjonalnie też klient (patrz opis)
long PORT;

// FIFO_SIZE - rozmiar w bajtach kolejki FIFO, którą serwer utrzymuje dla każdego z klientów; ustawiany parametrem -F serwera, domyślnie 10560
long FIFO_SIZE;

// FIFO_LOW_WATERMARK - opis w treści; ustawiany parametrem -L serwera, domyślnie 0
long FIFO_LOW_WATERMARK;

// FIFO_HIGH_WATERMARK - opis w treści; ustawiany parametrem -H serwera, domyślnie równy FIFO_SIZE
long FIFO_HIGH_WATERMARK;

// BUF_LEN - rozmiar (w datagramach) bufora pakietów wychodzących, ustawiany parametrem -X serwera, domyślnie 10
long BUF_LEN;

// TX_INTERVAL - czas (w milisekundach) pomiędzy kolejnymi wywołaniami miksera, ustawiany parametrem -i serwera; domyślnie: 5ms
long TX_INTERVAL;

long clients_counter = 0;

boost::asio::io_service ioService;

boost::asio::deadline_timer report_timer(ioService);

boost::asio::deadline_timer mixer_timer(ioService);

map<udp::endpoint, Client> clients;

// Forward declarations

bool setup(int ac, char* av[]);
void send_report();
void check_alive_clients();
void reporting_task();
void send_data();
void data_task();
void open_tcp();
void tcp_handler(const boost::system::error_code &error, size_t bytes_transferred);
void tcp_task();
void open_udp();
void udp_handler(const boost::system::error_code &error, size_t bytes_transferred);
void udp_task();

// Implementations

class Client
{
    public:
        long id;
        udp::endpoint endpoint;

        vector<char> fifo;
        bool isAlive = true;
        bool isActive = false; // zaczynamy od filling
        long min_since_report = FIFO_SIZE;
        long max_since_report = 0;
        long ack = 0;
        bool isUploading = false;
        bool should_delete = false;

        Client(long id, udp::endpoint endpoint)
        {
            id = id;
            endpoint = endpoint;
        }

        ~Client()
        {
            // pozamykaj sockety i inne rzeczy, cancel na timerach o ile jakies sa
        }

        int example_method()
        {
            return 0;
        }
};

bool setup(int ac, char* av[])
{
    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("port,p", po::value<long>(&PORT)
                  ->default_value(10000 + 334678 % 10000),
                  "listen on a port")
            ("fifo-size,F", po::value<long>(&FIFO_SIZE)
                  ->default_value(10560),
                  "Size of FIFO in use")
            ("fifo-low-watermark,L", po::value<long>(&FIFO_LOW_WATERMARK)
                  ->default_value(0),
                  "")
            ("fifo-high-watermark,H", po::value<long>(&FIFO_HIGH_WATERMARK)
                  ->default_value(0),
                  "")
            ("buf-len,X", po::value<long>(&BUF_LEN)
                  ->default_value(10),
                  "")
            ("tx-interval,i", po::value<long>(&TX_INTERVAL)
                  ->default_value(5),
                  "Sound transmission interval")
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

        if (vm["fifo-high-watermark"].defaulted())
            FIFO_HIGH_WATERMARK = FIFO_SIZE;

        cerr << "Server configuration:\n";
        cerr << "PORT: " << PORT;
        cerr << "\nFIFO_SIZE " << FIFO_SIZE;
        cerr << "\nFIFO_LOW_WATERMARK " << FIFO_LOW_WATERMARK;
        cerr << "\nFIFO_HIGH_WATERMARK " << FIFO_HIGH_WATERMARK;
        cerr << "\nBUF_LEN " << BUF_LEN;
        cerr << "\nTX_INTERVAL " << TX_INTERVAL << '\n';

        return true;
    }
    catch(std::exception& e)
    {
        cerr << e.what() << "\n";
        return false;
    }
}

void send_report()
{
    for (auto client : clients)
    {
        cerr << client.second.max_since_report << client.second.min_since_report << client.second.fifo.size();
        client.second.max_since_report = 0;
        client.second.min_since_report = FIFO_SIZE;

        if (!client.second.isAlive)
        {
            delete &client.second;
            clients.erase(client.first);
        }
        else
            client.second.isAlive = false;
    }
}

void check_alive_clients()
{}

void reporting_task()
{}

void send_data()
{}

void data_task()
{}

void open_tcp()
{}

void tcp_handler(const boost::system::error_code &error, size_t bytes_transferred)
{}

void tcp_task()
{}

void open_udp()
{}

void udp_handler(const boost::system::error_code &error, size_t bytes_transferred)
{}

void udp_task()
{}

int main(int ac, char* av[])
{
    if (! setup(ac, av))
    {
        cout << "Error parsing server configuration\n";
        exit(0);
    }

    open_tcp();
    cerr << "Accepting TCP connections\n";

    open_udp();
    cerr << "Accepting UDP connections\n";

    tcp_task();
    cerr << "Started reading TCP socket\n";

    udp_task();
    cerr << "Started reading UDP socket\n";

    data_task();
    cerr << "Started transmitting data to clients\n";

    reporting_task();
    cerr << "Started sending reports to clients\n";

    ioService.run();
    return 0;
}