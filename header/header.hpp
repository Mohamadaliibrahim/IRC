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
#include <sstream>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <algorithm>
#include <csignal>
#include <ctime>

// struct privmsg
// {
//     std::vector<std::string> channels;
//     std::vector<std::string> users;
//     bool    its_channel;
//     bool    its_user;
//     std::string message;
// };

extern int g_s;

struct Channel
{
    int                 superUser; // initial user
    std::string         name;
    std::vector<int>    clients; // all users   
    std::vector<int>    normalUsers;
    std::vector<int>    admins;
    std::vector<int>    invited;
    std::string         topic;
    time_t creationTime;
    int IsInviteOnly;
    int IsThereAPass;
    std::string pass;
    int MembersLimit;
    int TopicLock;
};

struct ChanSecParse
{
    std::string chan;
    std::string pass;
};

struct Client
{
    bool        authenticated;
    std::string nickname;
    std::string username;
    std::string buffer;
    std::string realname;
    bool        pass_flag;
    bool        nick_flag;
    bool        user_flag;
    bool        all_set;
};
struct t_environment
{
    int port;
    std::string pass;
    int server_socket;
    int client_count;
    Client guests;
    std::map<int, Client> clients;
    std::map<std::string, Channel> channels;
};
std::vector<std::string> split_on_comma(const std::string &str);
std::string sanitize_message(const std::string &msg);
std::string trim_that_first(const std::string& str);
std::string trim_that_last(const std::string& str);
std::vector<std::string> split_on_backspash_n(const std::string &str);
std::vector<std::string> split_on_space(const std::string &str, char delimiter);
std::string get_msg(const std::string &buffer);
void handle_client(int client_socket, t_environment *env);
void    lets_do_it(char **av);
void server_loop(t_environment *env);
void ft_private_message(int client_socket, const std::string &buffer, t_environment *env);
void set_non_blocking(int sockfd);
void    create_env(char **av, t_environment *env);
int create_server_socket(int port);
void    ft_join(int client_socket, const std::string &buffer, t_environment *env);
Channel create_channel(std::string channel_name,int clientsocket);
void broadcast_message(const std::string &message, const std::string &channel_name, t_environment *env);
void    check_av(char **av);
std::string get_msg1(const std::string &buffer);
// void first_message(int new_client, t_environment *env, pollfd clients[]);
int parse_topic(std::string cmd, std::string &chan, std::string &top, int client_socket, t_environment *env);
void    topic_func(int client_sd, std::string cmd, t_environment *env);
int parse_invite(const std::string &cmd_line, std::string &nickname, std::string &channel, int client_socket, t_environment *env);
void invite_func(int client_sd, const std::string &cmd, t_environment *env);
std::string trim_that_last_with_flag(const std::string& str, char *x);
void kick_func(int client_sd, const std::string &cmd, t_environment *env);
void mode_func(int client_sd, const std::string &cmd, t_environment *env);
int parse_mode(const std::string &cmd_line, std::string &channel, std::string &modes, std::vector<std::string> &modeParams, int client_socket, t_environment *env);

#endif
