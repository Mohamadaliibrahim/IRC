#include "header.hpp"

Channel create_channel(std::string channel_name)
{
    Channel new_one;
    new_one.name = channel_name;
    new_one.clients = std::vector<int>();
    return (new_one);
}

std::string trim_that_last(const std::string& str)
{
    // Find the last non-space character
    size_t end = str.find_last_not_of(" \t\r\n");

    // If there's no non-space character, return an empty string
    if (end == std::string::npos)
        return "";

    // Return the substring from the beginning to the last non-space character
    return str.substr(0, end + 1);
}


void ft_join(int client_socket, const std::string &buffer, t_environment *env)
{
    if (buffer.length() < 6)
    {
        std::string error_msg = "Invalid JOIN command.\n";
        send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        return;
    }
    std::string trimmed_buffer = trim_that_last(buffer);
    std::string channel_name = trimmed_buffer.substr(5);
    if (channel_name[0] == '#')
    {
        bool already_in_channel = false;
        for (std::vector<int>::iterator it = env->channels[channel_name].clients.begin(); 
             it != env->channels[channel_name].clients.end(); ++it)
        {
            if (*it == client_socket)
            {
                already_in_channel = true;
                break;
            }
        }
        if (already_in_channel)
        {
            std::string already_in_channel_msg = "You are already in this channel.\n";
            send(client_socket, already_in_channel_msg.c_str(), already_in_channel_msg.size(), 0);
            return;
        }
        if (env->channels.find(channel_name) == env->channels.end())
        {
            env->channels[channel_name] = create_channel(channel_name);
            std::cout << env->clients[client_socket].nickname << " created new channel: " << channel_name << std::endl;
        }
        env->channels[channel_name].clients.push_back(client_socket);
        std::string join_message = env->clients[client_socket].nickname + " joined channel: " + channel_name + "\n";
        send(client_socket, join_message.c_str(), join_message.size(), 0);
        if (env->channels[channel_name].clients.size() > 1)
            broadcast_message(env->clients[client_socket].nickname + " has joined the channel: " + channel_name + "\n", channel_name, env);
        std::cout << "Clients in channel " << channel_name << ":" << std::endl;
        for (std::vector<int>::iterator it = env->channels[channel_name].clients.begin(); it != env->channels[channel_name].clients.end(); ++it)
        {
            int client_sock = *it;
            std::cout << env->clients[client_sock].nickname << std::endl;  // Print the nickname of each client
        }
    }
    else
    {
        std::string error_msg = "Channel names must start with '#'.\n";
        send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        return;
    }
}
