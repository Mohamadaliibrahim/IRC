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


typedef struct s_environment
{
    int     port;
    char    *pass;
} t_environment;


#endif