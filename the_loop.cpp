#include "header/header.hpp"

#define MAX_CLIENTS 100
extern bool g_stop;

void    first_message(int new_client, t_environment *env, pollfd clients[])
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(new_client, (struct sockaddr *)&client_addr, &addr_len) == -1)
    {
        std::cerr << "Error getting client address." << std::endl;
        return;
    } 
    std::cout << "New client #" << new_client
              << " from " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << std::endl;
    clients[++env->client_count].fd = new_client;
    clients[env->client_count].events = POLLIN;
    env->clients[new_client] = Client();
    env->clients[new_client].authenticated = false;
}

void server_loop(t_environment *env)
{
    struct pollfd clients[MAX_CLIENTS];
    std::string shutdownMsg;
    memset(clients, 0, sizeof(clients));
    int fd, poll_result, new_client;

    clients[0].fd = env->server_socket;
    clients[0].events = POLLIN;
    while (true)
    {
        if (g_stop)
        {
            // Gracefully disconnect everyone
            for (int i = 1; i <= env->client_count; i++)
            {
                fd = clients[i].fd;
                // IRC-like "ERROR" or "QUIT" message
                shutdownMsg = "ERROR :Server shutting down\r\n";
                shutdownMsg = sanitize_message(shutdownMsg);
                send(fd, shutdownMsg.c_str(), shutdownMsg.size(), MSG_NOSIGNAL);

                // Close file descriptor
                close(fd);

                // Also remove from env->clients map
                env->clients.erase(fd);
            }

            // Finally close the server
            close(env->server_socket);
            std::cout << "Server shutting down gracefully..." << std::endl;
            break;  // break out of the while loop
        }

        // ---------------------
        // normal poll loop code
        // ---------------------

        poll_result = poll(clients, env->client_count + 1, -1);
        if (poll_result == -1)
        {
            if (g_stop)
            {
                // Do the same cleanup if poll got interrupted by SIGINT
                for (int i = 1; i <= env->client_count; i++)
                {
                    fd = clients[i].fd;
                    shutdownMsg = "ERROR :Server shutting down\r\n";
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

        // Accept new connections
        if (clients[0].revents & POLLIN)
        {
            new_client = accept(env->server_socket, NULL, NULL);
            if (new_client == -1)
            {
                std::cerr << "Error accepting client." << std::endl;
                continue;
            }
            set_non_blocking(new_client);
            first_message(new_client, env, clients);
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
