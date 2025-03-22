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

void    create_env(char **av, t_environment *env)
{
    env->port = std::atoi(av[1]);
    env->pass = av[2];
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

    if (listen(server_socket, MAX_CLIENTS) == -1) //Prepare the socket to listen for incoming connections
    {
        std::cerr << "Error listening on socket." << std::endl;
        exit(1);
    }

    return server_socket;
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

void handle_client(int client_socket)
{
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0)
    {
        std::cout << "Received: " << buffer << std::endl;
        send(client_socket, buffer, bytes_received, 0);  // Echo back the message
    }
    else if (bytes_received == 0)
    {
        std::cout << "Client disconnected." << std::endl;
        close(client_socket);
    }
    else
    {
        std::cerr << "Error receiving data from client." << std::endl;
    }
}

void server_loop(int server_socket)
{
    struct pollfd clients[MAX_CLIENTS];
    int client_count = 0;

    // Add server socket to poll
    clients[0].fd = server_socket;
    clients[0].events = POLLIN;

    while (true)
    {
        int poll_result = poll(clients, client_count + 1, -1);  // Monitor server socket + clients
        if (poll_result == -1)
        {
            std::cerr << "Poll error." << std::endl;
            exit(1);
        }

        // Check if new client is trying to connect
        if (clients[0].revents & POLLIN)
        {
            int new_client = accept(server_socket, NULL, NULL);
            if (new_client == -1)
            {
                std::cerr << "Error accepting client." << std::endl;
                continue;
            }

            set_non_blocking(new_client);

            // Add new client to poll array
            clients[++client_count].fd = new_client;
            clients[client_count].events = POLLIN;
            std::cout << "New client connected." << std::endl;
        }

        // Handle communication with existing clients
        for (int i = 1; i <= client_count; ++i)
        {
            if (clients[i].revents & POLLIN)
            {
                handle_client(clients[i].fd);
            }
        }
    }
}

void    lets_do_it(char **av)
{
    t_environment env;

    check_av(av);
    create_env(av, &env);
    int server_socket = create_server_socket(env.port);
    server_loop(server_socket);
    close(server_socket);
//     std::cout<< server_socket << std::endl;
//     std::cout << "Port: " << env.port << "\nPassword: " << env.pass << std::endl;
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