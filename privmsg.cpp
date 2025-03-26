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
    // Parse the target (recipient) and message
    size_t target_end = buffer.find(" ", 8);  // Find the space after "PRIVMSG"
    std::string target = buffer.substr(8, target_end - 8);  // Extract the target nickname
    std::string msg = get_msg(buffer);  // Extract the message after the target

    // If the target is a channel, handle it as a channel message
    if (target[0] == '#')  // Check if the target is a channel
    {
        if (env->channels.find(target) != env->channels.end())  // If the channel exists
        {
            bool sender_in_channel = false;
            // Check if the sender is in the channel
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

            // If the sender is in the channel, broadcast the message
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
    else  // If the target is a user (not a channel)
    {
        bool recipient_found = false;
        // Iterate through all clients to find the recipient
        for (std::map<int, Client>::iterator it = env->clients.begin(); it != env->clients.end(); ++it)
        {
            if (it->second.nickname == target)  // If the recipient is found
            {
                std::string private_msg = env->clients[client_socket].nickname + " (private): " + msg + "\n";
                send(it->first, private_msg.c_str(), private_msg.size(), 0);  // Send the private message
                recipient_found = true;
                break;
            }
        }
        // If the recipient is not found, send an error to the sender
        if (!recipient_found)
        {
            std::string error_msg = "No such user: " + target + "\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }
    }
}

