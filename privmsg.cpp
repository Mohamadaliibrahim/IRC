#include "header.hpp"

std::string get_msg(const std::string &buffer)
{
    size_t target_end = buffer.find(" ", 8);
    size_t colon_pos = buffer.find(" :", target_end);
    std::string msg;

    if (colon_pos != std::string::npos) // If there's a colon, take everything after it
        msg = buffer.substr(colon_pos + 2);  // Everything after ":"
    else // If there's no colon, take only the first word as the message
    {
        size_t msg_end = buffer.find(" ", target_end + 1);  // Find next space after the target
        if (msg_end != std::string::npos)
        {
            msg = buffer.substr(target_end + 1, msg_end - target_end - 1);  // Extract only the first word (e.g., "hello")
        }
        else
            msg = buffer.substr(target_end + 1);  // If no space, take the rest (handle edge case)
    }
    return msg;
}

void ft_private_message(int client_socket, const std::string &buffer, t_environment *env)
{
    size_t target_end = buffer.find(" ", 8);
    std::string target = buffer.substr(8, target_end - 8);
    std::string msg = get_msg(buffer);

    if (target[0] == '#')
    {
        if (env->channels.find(target) != env->channels.end())
        {
            bool sender_in_channel = false;
            std::vector<int>::iterator client_it = env->channels[target].clients.begin();
            while (client_it != env->channels[target].clients.end())
            {
                if (*client_it == client_socket)
                {
                    sender_in_channel = true;
                    break;
                }
                client_it++;
            }

            if (sender_in_channel)
                broadcast_message(env->clients[client_socket].nickname + " (channel): " + msg + "\n", target, env);
            else
            {
                std::string error_msg = "You are not in the channel: " + target + "\n";
                send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            }
        }
        else
        {
            std::string error_msg = "No such channel: " + target + "\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }
    }
    else
    {
        bool recipient_found = false;
        for (std::map<int, Client>::iterator it = env->clients.begin(); it != env->clients.end(); ++it)
        {
            if (it->second.nickname == target)
            {
                send(it->first, (env->clients[client_socket].nickname + " (private): " + msg + "\n").c_str(), msg.size() + env->clients[client_socket].nickname.size() + 13, 0);
                recipient_found = true;
                break;
            }
        }
        if (!recipient_found)
        {
            std::string error_msg = "No such user: " + target + "\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }
    }
}
