#include "header/header.hpp"

//initializing the channel on creation
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

//trim the punctuations from the end of the string
//in case of hexchat (removing \t\n\r)
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
    std::vector<std::string> chans, passs;
    int i = 0, chanf = 0, j;

    //making sure that the command is "JOIN " at least
    if (strncmp(cmd.c_str(), "JOIN ", 5) != 0)
    {
        return -3;
    }

    //parsing the channel name (or channels)
    i += 5;
    while (cmd[i] != '\0')
    {
        if (chanf == 0 && cmd[i] != '#')
        {
            return -3;
        }
        j = i;
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
                return -5;
            }
            chanf = 1;
        }
        if (cmd[i] != '\0')
            i++;
    }

    //in case only channels are given (without passwords)
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
        //in the other case (with passwords) parsing them and making sure that the
        //number of channels given is the same as the nm of channels
        if (passs.size() != chans.size())
        {
            return -5;
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
    int nf = 0, invitedf = 0, cf = 0;;//new flag and invited user flag
    std::vector<ChanSecParse> jnde;
    int res = parse_join(buffer, jnde);//getting the result from the parse function
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username;
    std::string serverName = "my.irc.server";
    std::ostringstream oss;
    std::string message, given_pass, channel_name;
    bool already_in_channel, is_admin;

    //in case there is a bas channel given
    if (res == -3)
    {
        oss << ":" << serverName << " 476 " << nick << " :JOIN :Bad Channel Mask\r\n";
        message = oss.str();
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
        return;
    }

    //in case not enough parameters are given
    if (res == -5)
    {
        oss << ":" << serverName << " 461 " << nick << " :JOIN not enough Parameters\r\n";
        message = oss.str();
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
    }

    //in case everything is correct, checking the channels, passwords and everything
    //with the functionality
    for (std::vector<ChanSecParse>::iterator it = jnde.begin(); it != jnde.end(); it++)
    {
        channel_name = trim_that_last(it->chan);
        given_pass = it->pass;
        
        channel_name = trim_that_first(channel_name);
        channel_name = trim_that_last(channel_name);

        already_in_channel = false;
        
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
            message = "You are already in " + channel_name + " channel.\n";
            message = sanitize_message(message);
            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
            continue;
        }
        cf = 0;
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
            // send MODE +o to channel creator and all present users
            message = ":" + env->clients[client_socket].nickname + "!" + env->clients[client_socket].username + "@localhost MODE " + channel_name + " +o " + env->clients[client_socket].nickname + "\r\n"; // *** CHANGED ***
            for (std::vector<int>::iterator itc = env->channels[channel_name].clients.begin(); itc != env->channels[channel_name].clients.end(); ++itc)                                           // *** CHANGED ***
                send(*itc, message.c_str(), message.size(), MSG_NOSIGNAL);                                                                                                                   // *** CHANGED ***
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
                    else
                    {
                        oss << ":" << serverName << " 471 " << nick << " " << channel_name << " :Cannot join channel (+l) - channel is full\r\n";
                        message = oss.str();
                        message = sanitize_message(message);
                        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
                        return ;
                    }
                }
            }
        }
        message = ":" + env->clients[client_socket].nickname + " JOIN " + channel_name + "\n";
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);

        if (!env->channels[channel_name].topic.empty())
        {
            message = ":server_name 332 " + env->clients[client_socket].nickname + " " + channel_name + " :" + env->channels[channel_name].topic + "\n";
            message = sanitize_message(message);
            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);

            std::stringstream topic_time_msg;
            topic_time_msg << ":server_name 333 " << env->clients[client_socket].nickname << " " << channel_name << " " << env->clients[client_socket].nickname << "\n";
            message = sanitize_message(topic_time_msg.str());
            send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);
        }

        message = ":server_name 353 " + env->clients[client_socket].nickname + " = " + channel_name + " :";
        for (std::vector<int>::iterator it_channel = env->channels[channel_name].clients.begin(); it_channel != env->channels[channel_name].clients.end(); ++it_channel)
        {
            is_admin = false;                                                                                                                        // *** CHANGED ***
            for (std::vector<int>::iterator it_ad = env->channels[channel_name].admins.begin(); it_ad != env->channels[channel_name].admins.end(); ++it_ad) // *** CHANGED ***
                if (*it_ad == *it_channel) { is_admin = true; break; }                                                                                    // *** CHANGED ***
            std::string prefix = is_admin ? "@" : "";                                                                                                     // *** CHANGED ***
            message += prefix + env->clients[*it_channel].nickname + " ";                                                                           // *** CHANGED ***
        }
        message += "\n";
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);

        message = ":server_name 366 " + env->clients[client_socket].nickname + " " + channel_name + " :End of /NAMES list.\n";
        message = sanitize_message(message);
        send(client_socket, message.c_str(), message.size(), MSG_NOSIGNAL);

        if (env->channels[channel_name].clients.size() > 1)
        {
            broadcast_message(env->clients[client_socket].nickname + " has joined the channel: " + channel_name + "\n", channel_name, env);
            // -------------- ADDED RFC-STYLE JOIN BROADCAST --------------
            message = 
                ":" + env->clients[client_socket].nickname +
                "!" + env->clients[client_socket].username +
                "@localhost JOIN :" + channel_name + "\r\n";

            for (std::vector<int>::iterator itc = env->channels[channel_name].clients.begin();
                 itc != env->channels[channel_name].clients.end(); ++itc)
            {
                if (*itc != client_socket)
                    send(*itc, message.c_str(), message.size(), MSG_NOSIGNAL);
            }
        }
    }
}
