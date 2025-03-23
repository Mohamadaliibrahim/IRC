#ifndef HEADER_HPP
#define HEADER_HPP

#include <iostream>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <map>
#include <vector>

struct Client {
    bool authenticated;
    // You can add more information per client, like nickname, etc.
};

struct t_environment {
    int port;
    std::string pass;
    int server_socket;
    int client_count;
    std::map<int, Client> clients;  // Map to store client authentication status
};



#endif