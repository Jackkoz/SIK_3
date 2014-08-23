#include <boost/program_options.hpp>

using namespace boost;
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <regex>

using namespace std;

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

class client
{
    private int example_variable;

    private int example_method()
    {
        return 0;
    }
};

void identifyMessage(String message)
{
    std::tr1::regex regex = "^CLIENT";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
    {
        // CLIENT clientid\n
        // Wysyłany przez klienta jako pierwszy datagram UDP. Parametr clientid powinien być taki
        // sam jak odebrany od serwera w połączeniu TCP.
        return;
    }

    regex = "^UPLOAD";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
    {
        // UPLOAD nr\n
        // [dane]
        // Wysyłany przez klienta datagram z danymi.
        // Nr to numer datagramu, zwiększany o jeden przy każdym kolejnym fragmencie danych.
        // Taki datagram może być wysłany tylko po uprzednim odebraniu wartości ack
        // (patrz ACK lub DATA) równej nr oraz nie może zawierać więcej danych niż ostatnio odebrana wartość win.
        return;
    }

    regex = "^RETRANSMIT";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
    {
        // RETRANSMIT nr\n
        // Wysyłana przez klienta do serwera prośba o ponowne przesłanie wszystkich dostępnych
        // datagramów o numerach większych lub równych nr.
        return;
    }

    regex = "^KEEPALIVE";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
    {
        // KEEPALIVE\n
        // Wysyłany przez klienta do serwera 10 razy na sekundę.
        return;
    }

}

int main(int ac, char* av[])
{
    try
    {
        int portnum;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("port,p", po::value<int>(&portnum)
                  ->default_value(10000 + 334678 % 10000,"no"),
                  "listen on a port.")
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
            cout << "port: "
                 << vm["port"].as<int>() << "\n";
        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }

    cout << "I'm working!" << endl;

    TCP_socket = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket

    cout << "Accepting TCP connections" << endl;

    return 0;
}