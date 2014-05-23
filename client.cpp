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

// Numery portów klienta powinny być dynamicznie przydzielone przez system.

// Połącz przez TCP

// Połącz przez UDP

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