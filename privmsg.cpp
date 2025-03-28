#include "header.hpp"

std::string trim_that_last_with_flag(const std::string& str, char *x)
{
    size_t end = str.find_last_not_of(" \t\r\n");

    if (end == std::string::npos)
        return "";
    if (end != (str.size() - 1))
        (*x) = 'a';
    return str.substr(0, end + 1);
}

void ft_private_message(int client_socket, const std::string &buffer, t_environment *env)
{
    char a = '\0';
    std::string trimmed_buffer = trim_that_last_with_flag(buffer, &a);
    std::vector<std::string> buff = split_on_space(trimmed_buffer, ' ');
    std::vector<std::string>::iterator it = buff.begin();

    if (buff.size() <= 2)
    {
        std::string error_msg = "Error: PRIVMSG\n";
        error_msg = sanitize_message(error_msg);
        send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        return;
    }
    *it = trim_that_first(*it);
    *it = trim_that_last(*it);
    if ((*(it + 1))[0] == '#')
    {
        if (env->channels.find(*(it + 1)) != env->channels.end())  // If the channel exists
        {
            bool sender_in_channel = false;

            std::vector<int>::iterator client_it = env->channels[*(it + 1)].clients.begin();
            while (client_it != env->channels[*(it + 1)].clients.end())
            {
                if (*client_it == client_socket)
                {
                    sender_in_channel = true;
                    break;
                }
                client_it++;
            }
            std::string msg;
            std::vector<std::string>::iterator ix = it + 2;
            if (((((*ix)[0]) == ':') && ((*ix)[0]) != ' ') && (a == 'a'))
                ix[0] = (*ix).substr(1);
            while (ix != buff.end())
            {
                msg += *ix + " ";
                ix++;
            }
            if (!msg.empty())
            {
                msg.erase(msg.size() - 1);
            }
            if (sender_in_channel)
                broadcast_message(env->clients[client_socket].nickname + " (channel): " + msg + "\n", *(it + 1), env);
            else
            {
                std::string error_msg = "You are not in the channel: " + *(it + 1) + "\n";
                error_msg = sanitize_message(error_msg);
                send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            }
        }
        else
        {
            std::string error_msg = "No such channel: " + *(it + 1) + "\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }
    }
    else
    {
        std::string msg;
        bool recipient_found = false;
        std::vector<std::string>::iterator ix = it + 2;
        if (((((*ix)[0]) == ':') && ((*ix)[0]) != ' ') && (a == 'a'))
            ix[0] = (*ix).substr(1);
        while (ix != buff.end())
        {
            msg += *ix + " ";
            ix++;
        }
        if (!msg.empty())
        {
            msg.erase(msg.size() - 1);
        }
        it++;
        for (std::map<int, Client>::iterator ip = env->clients.begin(); ip != env->clients.end(); ++ip)
        {
            if (ip->second.nickname == (*it))
            {
                std::string private_msg = env->clients[client_socket].nickname + " (private): " + msg + "\n";
                private_msg = sanitize_message(private_msg);
                send(ip->first, private_msg.c_str(), private_msg.size(), 0);
                recipient_found = true;
                break;
            }
        }
        if (!recipient_found)
        {
            std::string error_msg = "No such user: " + *it + "\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }
    }
}

