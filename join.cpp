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
    new_one.IsInviteOnly = 0;
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

int parse_join(const std::string &cmd, std::vector<ChanSecParse> &parseData)
{
    ChanSecParse tmp;
    std::vector<std::string> chans;
    std::vector<std::string> passs;
    int i = 0;
    int chanf = 0;

    if (strncmp(cmd.c_str(), "JOIN ", 5) != 0)
    {
        return -3;
    }
    i += 5;
    while (cmd[i] != '\0')
    {
        if (chanf == 0 && cmd[i] != '#')
        {
            return -4;
        }
        int j = i;
        while (!isspace(cmd[j]) && cmd[j] != '\0' && cmd[j] != ',')
            j++;
        if (chanf == 0)
            chans.push_back(cmd.substr(i, j - i));
        else
            passs.push_back(cmd.substr(i, j - i));
        i = j;
        if (cmd[i] != '\0' && isspace(cmd[i]))
        {
            if (chanf == 1)
            {
                std::cout << "MIGGA TOO MANY ARGUMENTS\n";
                return -5;
            }
            chanf = 1;
        }
        if (cmd[i] != '\0')
            i++;
    }

    if (passs.size() == 0)
    {
        for (int i = 0; i < (int)chans.size(); i++)
        {
            tmp.chan = chans[i];
            parseData.push_back(tmp);
        }
        return 1;
    }
    else
    {
        if (passs.size() != chans.size())
        {
            std::cout << "THE NUMBER OF CHANNELS AND PASSWORDS DOES NOT MATCH\n";
            return -2;
        }
        for (int i = 0; i < (int)chans.size(); i++)
        {
            tmp.chan = chans[i];
            tmp.pass = passs[i];
            parseData.push_back(tmp);
        }
        return 2;
    }
}

void ft_join(int client_socket, const std::string &buffer, t_environment *env)
{
    int nf = 0, invitedf = 0;
    std::vector<ChanSecParse> jnde;
    int res = parse_join(buffer, jnde);
    // std::vector<std::string> channels = split_on_comma(buffer);
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username;
    std::string serverName = "my.irc.server";
    std::ostringstream oss;
    std::string message;
    if (res == -3)
    {
        oss << ":" << serverName << " 476 " << nick << " :JOIN :Bad Channel Mask\r\n";
        message = oss.str();
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
        return;
    }
    if (res == -4)
    {
        oss << ":" << serverName << " 476 " << nick << " :JOIN :Bad Channel Mask\r\n";
        message = oss.str();
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
        return ;
    }
    if (res == -5)
    {
        oss << ":" << serverName << " 461 " << nick << " :JOIN not enough Parameters\r\n";
        message = oss.str();
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
    }
    for (std::vector<ChanSecParse>::iterator it = jnde.begin(); it != jnde.end(); it++)
    {
        std::string channel_name = trim_that_last(it->chan);
        std::string given_pass = it->pass;
        
        channel_name = trim_that_first(channel_name);
        channel_name = trim_that_last(channel_name);

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
            std::cout << channel_name << "jnde channel" << std::endl;
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
        std::cout << (int)env->channels[channel_name].IsInviteOnly << std::endl;
        if (nf == 1)
        {
            env->channels[channel_name].superUser = client_socket;//set the channel creator as superuser
            env->channels[channel_name].admins.push_back(client_socket);// add the superuser to the admin list
            // send MODE +o to channel creator and all present users                        // *** CHANGED ***
            std::string mode_msg = ":" + env->clients[client_socket].nickname + "!" + env->clients[client_socket].username + "@localhost MODE " + channel_name + " +o " + env->clients[client_socket].nickname + "\r\n"; // *** CHANGED ***
            for (std::vector<int>::iterator itc = env->channels[channel_name].clients.begin(); itc != env->channels[channel_name].clients.end(); ++itc)                                           // *** CHANGED ***
                send(*itc, mode_msg.c_str(), mode_msg.size(), MSG_NOSIGNAL);                                                                                                                   // *** CHANGED ***
        }
        else if  (env->channels[channel_name].IsInviteOnly == 1) // is invited only
        {
            for (int i = 0; i < (int)env->channels[channel_name].invited.size(); i++)
            {
                if (env->channels[channel_name].invited[i] == client_socket)
                    invitedf = 1;
            }
            if (invitedf == 1) // invited 
            {
                if (env->channels[channel_name].MembersLimit == -1) // no limit
                {
                    if (env->channels[channel_name].IsThereAPass == 1) // pass 
                    {
                        if (given_pass ==  env->channels[channel_name].pass) // correct pass
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                            env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                        else // incorrect pass
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                            message = oss.str();
                            message = sanitize_message(message);
                            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                            return ;
                        }
                    }
                    else // not pass and no limit
                    {
                        env->channels[channel_name].clients.push_back(client_socket);
                        env->channels[channel_name].normalUsers.push_back(client_socket);
                        env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                    }
                }
                else  // with limit
                {
                    if ((int)env->channels[channel_name].clients.size() < env->channels[channel_name].MembersLimit) // free space to join
                    {   
                        if (env->channels[channel_name].IsThereAPass == 1) // pass 
                        {
                            if (given_pass ==  env->channels[channel_name].pass)  // correct pass
                            {
                                env->channels[channel_name].clients.push_back(client_socket);
                                env->channels[channel_name].normalUsers.push_back(client_socket);
                                env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                            }
                            else // incorrect pass
                            {
                                oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                                message = oss.str();
                                message = sanitize_message(message);
                                send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                                return ;
                            }
                        }
                        else // no pass and available space
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                            env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                    }
                    else // limit reached
                    {
                        std::cout << "limits has been reached  in the invite only and invited " << std::endl; // to be removed 
                        oss << ":" << serverName << " 471 " << nick << " " << channel_name << " :Cannot join channel (+l) - channel is full\r\n";
                        message = oss.str();
                        message = sanitize_message(message);
                        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                        return ;
                    }
                }
            }
            else // not invited
            {
                oss << ":" << serverName << " 473 " << nick  << " " << channel_name << " :Cannot join channel (+i) - invite only\r\n";
                message = oss.str();
                message = sanitize_message(message);
                send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                return ;
            }
        }
        else // is not invite only
        {
            for (int i = 0; i < (int)env->channels[channel_name].invited.size(); i++) //  check if invited 
            {
                if (env->channels[channel_name].invited[i] == client_socket)
                    invitedf = 1;
            }
            if (invitedf == 1) // invited 
            {
                if (env->channels[channel_name].MembersLimit == -1) // no limits
                {
                    if(env->channels[channel_name].IsThereAPass == 1) // with pass
                    {
                        if (given_pass ==  env->channels[channel_name].pass)
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                            env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                        else 
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                            message = oss.str();
                            message = sanitize_message(message);
                            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                            return ;
                        }
                    }
                    else // not pass and no limit
                    {
                        env->channels[channel_name].clients.push_back(client_socket);
                        env->channels[channel_name].normalUsers.push_back(client_socket);
                        env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                    }
                }
                else // limits
                {
                    if (((int)env->channels[channel_name].clients.size() < env->channels[channel_name].MembersLimit)) // free spaces to join
                    {
                        if(env->channels[channel_name].IsThereAPass == 1) // pass required
                        {
                            if (given_pass ==  env->channels[channel_name].pass) // correct pass
                            {
                                env->channels[channel_name].clients.push_back(client_socket);
                                env->channels[channel_name].normalUsers.push_back(client_socket);
                                 env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                            }
                            else // incorrect pass
                            {
                                oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                                message = oss.str();
                                message = sanitize_message(message);
                                send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                                return ;
                            }
                        }
                        else // not pass and no limit
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                            env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                    }
                    else // limit reach
                    {
                        std::cout << "limits has been reached Invited but there is no  INVITE only restriction" << std::endl;
                        oss << ":" << serverName << " 471 " << nick << " " << channel_name << " :Cannot join channel (+l) - channel is full\r\n";
                        message = oss.str();
                        message = sanitize_message(message);
                        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                        return ;
                    }
                }
            }
            else // not in the invited vector 
            {
                if (env->channels[channel_name].MembersLimit == -1)// no limit
                {
                    if(env->channels[channel_name].IsThereAPass == 1) //check key
                    {
                        if (given_pass ==  env->channels[channel_name].pass)  // correct pass
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                             env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                        else // incorrect pass
                        {
                            oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                            message = oss.str();
                            message = sanitize_message(message);
                            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                            return ;
                        }
                    }
                    else // no key and no limit
                    {
                        env->channels[channel_name].clients.push_back(client_socket);
                        env->channels[channel_name].normalUsers.push_back(client_socket);
                        env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                    }
                }
                else // limit
                {
                    if (((int)env->channels[channel_name].clients.size() < env->channels[channel_name].MembersLimit)) // free space to join
                    {
                        if(env->channels[channel_name].IsThereAPass == 1) // pass required
                        {
                            if (given_pass ==  env->channels[channel_name].pass)  // correct pass
                            {
                                env->channels[channel_name].clients.push_back(client_socket);
                                env->channels[channel_name].normalUsers.push_back(client_socket);
                                 env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                            }
                            else // incorrect pass
                            {
                                oss << ":" << serverName << " 475 " << nick << " " << channel_name << " :Cannot join channel (+k) - incorrect pass\r\n";
                                message = oss.str();
                                message = sanitize_message(message);
                                send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                                return ;
                            }
                        }
                        else // no key and no limit
                        {
                            env->channels[channel_name].clients.push_back(client_socket);
                            env->channels[channel_name].normalUsers.push_back(client_socket);
                            env->channels[channel_name].invited.erase(std::remove(env->channels[channel_name].invited.begin(), env->channels[channel_name].invited.end(), client_socket), env->channels[channel_name].invited.end());
                        }
                    }   
                    else // limit reached
                    {
                        std::cout << "limits has been reached NOT invited  and  there is no  INVITE only restriction " << std::endl;
                        oss << ":" << serverName << " 471 " << nick << " " << channel_name << " :Cannot join channel (+l) - channel is full\r\n";
                        message = oss.str();
                        message = sanitize_message(message);
                        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                        return ;
                    }
                }
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
            // prefix '@' for channel operators                                                                                                            // *** CHANGED ***
            bool is_admin = false;                                                                                                                        // *** CHANGED ***
            for (std::vector<int>::iterator it_ad = env->channels[channel_name].admins.begin(); it_ad != env->channels[channel_name].admins.end(); ++it_ad) // *** CHANGED ***
                if (*it_ad == *it_channel) { is_admin = true; break; }                                                                                    // *** CHANGED ***
            std::string prefix = is_admin ? "@" : "";                                                                                                     // *** CHANGED ***
            user_list_msg += prefix + env->clients[*it_channel].nickname + " ";                                                                           // *** CHANGED ***
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
