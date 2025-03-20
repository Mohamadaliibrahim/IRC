#include <iostream>

int main(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
        return 1;
    }
    std::cout << "Port: " << av[1] << "\nPassword: " << av[2] << std::endl;

    return 0;
}