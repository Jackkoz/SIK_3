#include <boost/program_options.hpp>

using namespace boost;
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <regex>

using namespace std;

class Client
{
    private:
        int example_variable;

        int example_method()
        {
            return 0;
        }
};

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

map<long, Client> clients;

void identifyMessage(string message)
{
    std::regex regex("^CLIENT");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // CLIENT clientid\n
        // Wysyłany przez klienta jako pierwszy datagram UDP. Parametr clientid powinien być taki
        // sam jak odebrany od serwera w połączeniu TCP.
        return;
    }

    regex = std::regex("^UPLOAD");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // UPLOAD nr\n
        // [dane]
        // Wysyłany przez klienta datagram z danymi.
        // Nr to numer datagramu, zwiększany o jeden przy każdym kolejnym fragmencie danych.
        // Taki datagram może być wysłany tylko po uprzednim odebraniu wartości ack
        // (patrz ACK lub DATA) równej nr oraz nie może zawierać więcej danych niż ostatnio odebrana wartość win.
        return;
    }

    regex = std::regex("^RETRANSMIT");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // RETRANSMIT nr\n
        // Wysyłana przez klienta do serwera prośba o ponowne przesłanie wszystkich dostępnych
        // datagramów o numerach większych lub równych nr.
        return;
    }

    regex = std::regex("^KEEPALIVE");
    if (std::regex_search(message.begin(), message.end(), regex))
    {
        // KEEPALIVE\n
        // Wysyłany przez klienta do serwera 10 razy na sekundę.
        return;
    }

}

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

int main(int ac, char* av[])
{
    try
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("port,p", po::value<long>(&PORT)
                  ->default_value(10000 + 334678 % 10000,"no"),
                  "listen on a port")
            ("fifo-size,F", po::value<long>(&FIFO_SIZE)
                  ->default_value(10560,"no"),
                  "Size of FIFO in use")
            ("fifo-low-watermark,L", po::value<long>(&FIFO_LOW_WATERMARK)
                  ->default_value(0,"no"),
                  "")
            ("fifo-high-watermark,H", po::value<long>(&FIFO_HIGH_WATERMARK)
                  ->default_value(FIFO_SIZE,"no"),
                  "")
            ("buf-len,X", po::value<long>(&BUF_LEN)
                  ->default_value(10,"no"),
                  "")
            ("tx-interval,i", po::value<long>(&TX_INTERVAL)
                  ->default_value(5,"no"),
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
            return 0;
        }

        if (vm.count("port"))
        {
            PORT = vm["port"].as<long>();
        }

        if (vm.count("fifo-size"))
        {
            FIFO_SIZE = vm["fifo-size"].as<long>();
            FIFO_HIGH_WATERMARK = vm["fifo-size"].as<long>();
        }

        if (vm.count("fifo-low-watermark"))
        {
            FIFO_LOW_WATERMARK = vm["fifo-low-watermark"].as<long>();
        }

        if (vm.count("fifo-high-watermark"))
        {
            FIFO_HIGH_WATERMARK = vm["fifo-high-watermark"].as<long>();
        }

        if (vm.count("buf-len"))
        {
            BUF_LEN = vm["buf-len"].as<long>();
        }

        if (vm.count("tx-interval"))
        {
            TX_INTERVAL = vm["tx-interval"].as<long>();
        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }

    cout << "I'm working!" << endl;

    // TCP_socket = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket

    cout << "Accepting TCP connections" << endl;

    return 0;
}