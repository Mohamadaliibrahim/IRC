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
    size_t end = str.find_last_not_of(" \t\r\n");

    if (end == std::string::npos)
        return "";

    return str.substr(0, end + 1);
}

std::vector<std::string> split_on_comma(const std::string &str)
{
    std::vector<std::string> result;
    std::string word;
    std::istringstream stream(str);

    while (std::getline(stream, word, ','))
    {
        word.erase(std::remove(word.begin(), word.end(), '\r'), word.end());
        if (!word.empty())
            result.push_back(word);
    }

    return result;
}

//JOIN #general,#chatroom

#include "header.hpp"

void ft_join(int client_socket, const std::string &buffer, t_environment *env)
{
    std::vector<std::string> channels = split_on_comma(buffer);
    if (channels.empty())
    {
        std::string error = "Invalid JOIN command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    for (std::vector<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        std::string channel_name = trim_that_last(*it);
        if (strncmp((*it).c_str(), "JOIN", 4) == 0)
        {
            std::vector<std::string> a = split_on_space(*it, ' ');
            channel_name = a[1];
        }
        
        channel_name = trim_that_first(channel_name);
        channel_name = trim_that_last(channel_name);

        if (channel_name.empty() || channel_name[0] != '#') 
        {
            std::string error = "Invalid channel name.\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), 0);
            continue;
        }

        bool already_in_channel = false;
        
        for (std::vector<int>::iterator it_channel = env->channels[channel_name].clients.begin(); 
             it_channel != env->channels[channel_name].clients.end(); ++it_channel)
        {
            if (*it_channel == client_socket)
            {
                already_in_channel = true;
                break;
            }
        }

        if (already_in_channel)
        {
            std::string already_in_channel_msg = "You are already in " + channel_name + " channel.\n";
            already_in_channel_msg = sanitize_message(already_in_channel_msg);
            send(client_socket, already_in_channel_msg.c_str(), already_in_channel_msg.size(), 0);
            continue;
        }

        // Create channel if it doesn't exist
        if (env->channels.find(channel_name) == env->channels.end())
        {
            env->channels[channel_name] = create_channel(channel_name);
            env->channels[channel_name].superUser = client_socket;//set the channel creator as superuser
            env->channels[channel_name].admins.push_back(client_socket);// add the superuser to the admin list
            std::cout << env->clients[client_socket].nickname << " created new channel: " << channel_name << std::endl;
        }

        // Add the client to the channel
        env->channels[channel_name].clients.push_back(client_socket);

        // Send JOIN message (source: server_name, channel: channel_name)
        std::string join_msg = ":" + env->clients[client_socket].nickname + " JOIN " + channel_name + "\n";
        join_msg = sanitize_message(join_msg);
        send(client_socket, join_msg.c_str(), join_msg.size(), 0);

        // Send topic information (RPL_TOPIC and RPL_TOPICWHOTIME)
        if (!env->channels[channel_name].topic.empty())
        {
            std::string topic_msg = ":server_name 332 " + env->clients[client_socket].nickname + " " + channel_name + " :" + env->channels[channel_name].topic + "\n";
            topic_msg = sanitize_message(topic_msg);
            send(client_socket, topic_msg.c_str(), topic_msg.size(), 0);

            std::string topic_time_msg = ":server_name 333 " + env->clients[client_socket].nickname + " " + channel_name + " " + env->clients[client_socket].nickname + " " + "1617414567" + "\n";
            topic_time_msg = sanitize_message(topic_time_msg);
            send(client_socket, topic_time_msg.c_str(), topic_time_msg.size(), 0);
        }

        // Send the list of users in the channel (RPL_NAMREPLY and RPL_ENDOFNAMES)
        std::string user_list_msg = ":server_name 353 " + env->clients[client_socket].nickname + " = " + channel_name + " :";
        for (std::vector<int>::iterator it_channel = env->channels[channel_name].clients.begin(); it_channel != env->channels[channel_name].clients.end(); ++it_channel)
        {
            user_list_msg += env->clients[*it_channel].nickname + " ";
        }
        user_list_msg += "\n";
        user_list_msg = sanitize_message(user_list_msg);
        send(client_socket, user_list_msg.c_str(), user_list_msg.size(), 0);

        // Send end of names list message
        std::string end_of_names_msg = ":server_name 366 " + env->clients[client_socket].nickname + " " + channel_name + " :End of /NAMES list.\n";
        end_of_names_msg = sanitize_message(end_of_names_msg);
        send(client_socket, end_of_names_msg.c_str(), end_of_names_msg.size(), 0);

        // Broadcast the new client joining the channel to others in the channel
        if (env->channels[channel_name].clients.size() > 1)
        {
            broadcast_message(env->clients[client_socket].nickname + " has joined the channel: " + channel_name + "\n", channel_name, env);
        }
    }
}
