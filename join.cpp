#include "header.hpp"

Channel create_channel(std::string channel_name,int clientsocket)
{
    Channel new_one;
    new_one.name = channel_name;
    new_one.clients = std::vector<int>();
    new_one.admins = std::vector<int>();
    new_one.normalUsers = std::vector<int>();
    new_one.invited = std::vector<int>();
    new_one.superUser = clientsocket;
    new_one.topic = "";
    new_one.IsInviteOnly = -1;
    new_one.IsThereAPass = -1;
    new_one.pass = "";
    new_one.TopicLock = -1;
    new_one.MembersLimit = -1;
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
    int nf = 0, invitedf = 0;
    std::vector<std::string> channels = split_on_comma(buffer);
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username;
    std::string serverName = "my.irc.server";
    std::ostringstream oss;
    if (channels.empty())
    {
        std::string error = "Invalid JOIN command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
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
            send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
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
            send(client_socket, already_in_channel_msg.c_str(), already_in_channel_msg.size(), MSG_NOSIGNAL);
            continue;
        }

        // int jnde = 0;
        // // Create channel if it doesn't exist
        // std::cout << "jnde before that shit: " << jnde << "\n";
        // if (env->channels.find(channel_name) == env->channels.end())
        // {
        //     jnde = 1;
        //     // std::cout << "jnde is here: " << client_socket << "\n";
        //     env->channels[channel_name] = create_channel(channel_name,client_socket);
        //     // env->channels[channel_name].superUser = client_socket;//set the channel creator as superuser
        //     // env->channels[channel_name].admins.push_back(client_socket);// add the superuser to the admin list
        //     std::cout << env->clients[client_socket].nickname << " created new channel: " << channel_name << std::endl;
            
        // }

        // std::cout << "jnde after that shit: " << jnde << "\n";

        int cf = 0;

        for (std::map<std::string, Channel>::iterator it = env->channels.begin(); it != env->channels.end(); ++it)
        {
            if (it->second.name == channel_name)
            {
                cf = 1;
                break;
            }
        }

        if (cf == 0)
        {
            env->channels[channel_name] = create_channel(channel_name,client_socket);
        }

        // for (std::map<std::string, Channel>::iterator it = env->channels.begin(); it != env->channels.end(); it++)
        // {
        //     if ()
        // }

        
        // Add the client to the channel
        if ((int)env->channels[channel_name].clients.size() == 0)
        {
            env->channels[channel_name].clients.push_back(client_socket);
            nf = 1;
        }
        if (nf == 1)
        {
            env->channels[channel_name].superUser = client_socket;//set the channel creator as superuser
            env->channels[channel_name].admins.push_back(client_socket);// add the superuser to the admin list
        }
        else if  (env->channels[channel_name].IsInviteOnly == 1)
        {
            for (int i = 0; i < (int)env->channels[channel_name].invited.size(); i++)
            {
                if (env->channels[channel_name].invited[i] == client_socket)
                    invitedf = 1;
            }
            if (invitedf == 1)
            {
                if ((int)env->channels[channel_name].clients.size() < env->channels[channel_name].MembersLimit && env->channels[channel_name].MembersLimit != -1)
                {
                    if (env->channels[channel_name].IsThereAPass == 1)
                    {
                        /*if () check the if the password is ==  env->channels[channel_name].pass
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                        }
                        else
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                        }
                        */
                    }
                    else 
                    {
                        oss  << ":" << serverName << " 322 " << nick << " " << channel_name << env->channels[channel_name].clients.size() << ":" << env->channels[channel_name].topic << "\r\n";
                        env->channels[channel_name].clients.push_back(client_socket);
                        env->channels[channel_name].normalUsers.push_back(client_socket);
                    }
                }
                else
                {
                    oss << ":" << serverName << " 471 " << nick << " " << channel_name << " :Cannot join channel (+l) - channel is full\r\n"; 
                }
            }
            else
            {
                    oss << ":" << serverName << " 473 " << nick  << " " << channel_name << " :Cannot join channel (+i) - invite only\r\n";
            }
        }
        else //not invite only
        {
            if (((int)env->channels[channel_name].clients.size() < env->channels[channel_name].MembersLimit) && env->channels[channel_name].MembersLimit != -1)
            {
                if (env->channels[channel_name].IsThereAPass == 1)
                {
                    /*if () check the if the password is ==  env->channels[channel_name].pass
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                        }
                        else
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                        }
                    */
                }
                else
                {
                    env->channels[channel_name].clients.push_back(client_socket);
                    env->channels[channel_name].normalUsers.push_back(client_socket);
                }
            }
            else
            {
               /*if () check the if the password is ==  env->channels[channel_name].pass
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                        }
                        else
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                        }
                */
               //else 
                env->channels[channel_name].clients.push_back(client_socket);
                env->channels[channel_name].normalUsers.push_back(client_socket);
            }
        }

        // Send JOIN message (source: server_name, channel: channel_name)
        std::string join_msg = ":" + env->clients[client_socket].nickname + " JOIN " + channel_name + "\n";
        join_msg = sanitize_message(join_msg);
        send(client_socket, join_msg.c_str(), join_msg.size(), MSG_NOSIGNAL);

        // Send topic information (RPL_TOPIC and RPL_TOPICWHOTIME)
        if (!env->channels[channel_name].topic.empty())
        {
            std::string topic_msg = ":server_name 332 " + env->clients[client_socket].nickname + " " + channel_name + " :" + env->channels[channel_name].topic + "\n";
            topic_msg = sanitize_message(topic_msg);
            send(client_socket, topic_msg.c_str(), topic_msg.size(), MSG_NOSIGNAL);

            std::string topic_time_msg = ":server_name 333 " + env->clients[client_socket].nickname + " " + channel_name + " " + env->clients[client_socket].nickname + " " + "1617414567" + "\n";
            topic_time_msg = sanitize_message(topic_time_msg);
            send(client_socket, topic_time_msg.c_str(), topic_time_msg.size(), MSG_NOSIGNAL);
        }

        // Send the list of users in the channel (RPL_NAMREPLY and RPL_ENDOFNAMES)
        std::string user_list_msg = ":server_name 353 " + env->clients[client_socket].nickname + " = " + channel_name + " :";
        for (std::vector<int>::iterator it_channel = env->channels[channel_name].clients.begin(); it_channel != env->channels[channel_name].clients.end(); ++it_channel)
        {
            user_list_msg += env->clients[*it_channel].nickname + " ";
        }
        user_list_msg += "\n";
        user_list_msg = sanitize_message(user_list_msg);
        send(client_socket, user_list_msg.c_str(), user_list_msg.size(), MSG_NOSIGNAL);

        // Send end of names list message
        std::string end_of_names_msg = ":server_name 366 " + env->clients[client_socket].nickname + " " + channel_name + " :End of /NAMES list.\n";
        end_of_names_msg = sanitize_message(end_of_names_msg);
        send(client_socket, end_of_names_msg.c_str(), end_of_names_msg.size(), MSG_NOSIGNAL);

        // Broadcast the new client joining the channel to others in the channel
        if (env->channels[channel_name].clients.size() > 1)
        {
            broadcast_message(env->clients[client_socket].nickname + " has joined the channel: " + channel_name + "\n", channel_name, env);
            // -------------- ADDED RFC-STYLE JOIN BROADCAST --------------
            std::string join_broadcast_msg = 
                ":" + env->clients[client_socket].nickname +
                "!" + env->clients[client_socket].username +
                "@localhost JOIN :" + channel_name + "\r\n";

            // Send this JOIN line to everyone in the channel except the new user
            for (std::vector<int>::iterator itc = env->channels[channel_name].clients.begin();
                 itc != env->channels[channel_name].clients.end(); ++itc)
            {
                if (*itc != client_socket)
                    send(*itc, join_broadcast_msg.c_str(), join_broadcast_msg.size(), MSG_NOSIGNAL);
            }
            // -----------------------------------------------------------
        }
    }
}