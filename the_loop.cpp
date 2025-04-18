#include "header/header.hpp"

#define MAX_CLIENTS 100
extern bool g_stop;

void first_message(int new_client, const struct sockaddr_in& client_addr, t_environment* env, pollfd clients[])
{
    std::cout << "New client #" << new_client
    << " from " << inet_ntoa(client_addr.sin_addr)
    << ":"  << ntohs(client_addr.sin_port) << std::endl;

    clients[++env->client_count].fd     = new_client;
    clients[ env->client_count ].events = POLLIN;

    env->clients[new_client]               = Client();
    env->clients[new_client].authenticated = false;
}

void server_loop(t_environment *env)
{
    struct pollfd clients[MAX_CLIENTS];
    memset(clients, 0, sizeof(clients));

    clients[0].fd = env->server_socket;
    clients[0].events = POLLIN;

    while (true)
    {
        if (g_stop)
        {
            for (int i = 1; i <= env->client_count; i++)
            {
                int fd = clients[i].fd;
                std::string shutdownMsg = "ERROR :Server shutting down\r\n";
                shutdownMsg = sanitize_message(shutdownMsg);
                send(fd, shutdownMsg.c_str(), shutdownMsg.size(), MSG_NOSIGNAL);
                close(fd);
                env->clients.erase(fd);
            }
            close(env->server_socket);
            std::cout << "Server shutting down gracefully..." << std::endl;
            break;
        }

        int poll_result = poll(clients, env->client_count + 1, -1);
        if (poll_result == -1)
        {
            if (g_stop)
            {
                for (int i = 1; i <= env->client_count; i++)
                {
                    int fd = clients[i].fd;
                    std::string shutdownMsg = "ERROR :Server shutting down\r\n";
                    send(fd, shutdownMsg.c_str(), shutdownMsg.size(), MSG_NOSIGNAL);
                    close(fd);
                    env->clients.erase(fd);
                }
                close(env->server_socket);
                std::cout << "Server shutting down gracefully..." << std::endl;
                break;
            }
            std::cerr << "Poll error." << std::endl;
            exit(1);
        }
        if (clients[0].revents & POLLIN) //new client
        {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_client = accept(env->server_socket, (struct sockaddr *)&client_addr, &addr_len);
            if (new_client == -1)
            {
                std::cerr << "Error accepting client." << std::endl;
                continue;
            }
            set_non_blocking(new_client);
            first_message(new_client, client_addr, env, clients);
        }

        // Handle existing clients
        for (int i = 1; i <= env->client_count; ++i)
        {
            if (clients[i].revents & POLLIN)
            {
                handle_client(clients[i].fd, env);
            }
        }
    }
}
