#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>

// write value to sysctl file safely
void set_cc_algorithm(const std::string& algo)
{
    std::ofstream file("/proc/sys/net/ipv4/tcp_congestion_control");

    if (!file) {
        std::cerr << "Permission denied. Run as root.\n";
        return;
    }

    file << algo;
    file.close();

    std::cout << "Switched to: " << algo << std::endl;
}

// display current algorithm
void show_current()
{
    std::ifstream file("/proc/sys/net/ipv4/tcp_congestion_control");

    std::string algo;
    file >> algo;

    std::cout << "Current CC: " << algo << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "./ccctl show\n";
        std::cout << "./ccctl set <algorithm>\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "show")
        show_current();

    else if (cmd == "set" && argc == 3)
        set_cc_algorithm(argv[2]);

    else
        std::cout << "Invalid command\n";

    return 0;
}
