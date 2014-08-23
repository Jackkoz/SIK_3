#include <boost/program_options.hpp>

using namespace boost;
namespace po = boost::program_options;

#include <iostream>
#include <iterator>

using namespace std;

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

long PORT;

long RETRANSMIT_LIMIT;

// Numery portów klienta powinny być dynamicznie przydzielone przez system.

void identifyMessage(String message)
{
    std::tr1::regex regex = "^DATA";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
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

    regex = "^ACK";
    if (std::tr1::regex_search(message.begin(), message.end(), regex))
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
            ("server,s", po::value< vector<string> >(),
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
            return 0;
        }

        if (vm.count("port"))
        {
            cout << "port: "
                 << vm["port"].as<int>() << "\n";
        }

        if (vm.count("server"))
        {
            cout << "server: "
                 << vm["server"].as< vector<string> >() << "\n";
        }

        if (vm.count("retransmit-limit"))
        {

        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }

    cout << "Client reporting" << endl;

    //Przyjmuj dane ze standardowego wejścia i wrzucaj na serwer

    //zakończ przyjmowanie

    //Odbieraj dane od serwera aż do sigint

    return 0;
}