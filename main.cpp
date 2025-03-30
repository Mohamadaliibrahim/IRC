#include "header.hpp"

#define MAX_CLIENTS 100

void    check_av(char **av)
{
    for (int i = 0; av[1][i] != '\0'; ++i)
    {
        if (!std::isdigit(av[1][i]))
        {
            std::cerr << "Invalid port!" << std::endl;
            exit(1);
        }
    }
    int port = std::atoi(av[1]);
    if ((port < 1 || port > 65535) || (port >= 1 && port <= 1023))
    {
        std::cerr << "Invalid port!" << std::endl;
        exit(1);
    }
}

std::string sanitize_message(const std::string &msg)
{
    std::string result;
    for (size_t i = 0; i < msg.size(); i++)
    {
        if ((msg[i] >= 32 && msg[i] <= 126) || msg[i] == '\n')
        {
            result.push_back(msg[i]);
        }
    }
    return result;
}


void broadcast_message(const std::string &message, const std::string &channel_name, t_environment *env)
{
    std::map<std::string, Channel>::iterator it = env->channels.find(channel_name);
    if (it == env->channels.end())
        return;
    std::vector<int> &members = it->second.clients;
    for (std::size_t i = 0; i < members.size(); ++i)
    {
        send(members[i], message.c_str(), message.size(), 0);
    }
}


int create_server_socket(int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Error creating socket." << std::endl;
        exit(1);
    }
    struct sockaddr_in server_addr;//Defines a structure that holds the server's address information
    memset(&server_addr, 0, sizeof(server_addr));// Initializes the server_addr structure to zero, ensuring that it doesn't contain any garbage data
    server_addr.sin_family = AF_INET;//Specifies the address family as IPv4.
    server_addr.sin_addr.s_addr = INADDR_ANY;//Sets the IP address to INADDR_ANY, which means the server will
    //accept connections on any available network interface (i.e., it will listen for connections on all local network interfaces).
    server_addr.sin_port = htons(port);//Sets the server's port number. htons() (host-to-network short)
    //converts the port number to network byte order, ensuring compatibility across different systems.

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) //Associates the server socket with a specific address and port
    {
        std::cerr << "Error binding socket to port." << std::endl;
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1)
    {
        std::cerr << "Error listening on socket." << std::endl;
        exit(1);
    }
    return server_socket;
}

void    create_env(char **av, t_environment *env)
{
    env->port = std::atoi(av[1]);
    env->pass = av[2];
    env->server_socket = create_server_socket(env->port);
    env->client_count = 0;
}

void set_non_blocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "Error getting socket flags." << std::endl;
        exit(1);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Error setting socket to non-blocking." << std::endl;
        exit(1);
    }
}

void    lets_do_it(char **av)
{
    t_environment env;

    check_av(av);
    create_env(av, &env);
    server_loop(&env);
    close(env.server_socket);
}

int main(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
        return 1;
    }
    lets_do_it(av);

    return 0;
}