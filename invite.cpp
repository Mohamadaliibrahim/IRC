#include "header/header.hpp"

int parse_invite(const std::string &cmd_line, std::string &nickname, std::string &channel, int client_socket, t_environment *env)
{
    std::string serverName = "my.irc.server";
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username; 
    std::string host = "localhost"; 
    std::string cmd = cmd_line;
    int i = 0;
    //makes sure that the command is at least "INVITE "(7 charachters)
    if (cmd.size() < 7)
        return -1;
    if (strncmp(cmd.c_str(), "INVITE", 6) != 0)
        return -1;
    if (!isspace(cmd[6]))
        return -1;

    //skip spaces at the beginning and make sure there is parameters
    i = 6;
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
        return -1;

    //parsing the nickname of the invited user
    int s = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    nickname = cmd.substr(s, i - s);
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
        return -1;

    //skip the ':' in case of hexchat
    if (cmd[i] == ':')
        i++;

    //parsing the channel name with its validation
    s = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    channel = cmd.substr(s, i - s);
    if (channel.empty() || channel[0] != '#')
    {
        std::ostringstream oss;
       	oss << ":" << serverName << " 403 " << nick << " :INVITE :Channel name must start with a #\r\n";
        std::string error = oss.str();
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return -1;
    }
    return 0;
}
void invite_func(int client_sd, const std::string &cmd, t_environment *env)
{
    std::string serverName = "my.irc.server";
    std::string user = env->clients[client_sd].username; 
    std::string host = "localhost"; 
    std::string nick;
    std::string chan;
    int af = 0;//admin flag

    //filling the nickname of the invited user and the channel invited
    //to with syntax validation
    int parseResult = parse_invite(cmd, nick, chan, client_sd,env);
    if (parseResult == -1)
        return;

    //making sure that the channel exists
    if (env->channels.find(chan) == env->channels.end())
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 403 " << nick << " :INVITE :No such channel " << chan << "\r\n";
        std::string error  = oss.str();
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }

    int cf = 0;// client flag

    //making sure that the user inviting is a client in the channel
    for (size_t i = 0; i < env->channels[chan].clients.size(); i++)
    {
        if (env->channels[chan].clients[i] == client_sd)
        {
            cf = 1;
            break;
        }
    }
    if (!cf)
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 442 " << nick << " :INVITE :You're not on that channel\r\n";
        std::string error = oss.str();
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }

    int t_sd = -1;// the invited user descriptor 

    //getting the descriptor of the invited user with validation
    for (std::map<int, Client>::iterator it = env->clients.begin(); it != env->clients.end(); it++)
    {
        if (it->second.nickname == nick)
        {
            t_sd = it->first;
            break;
        }
    }
    if (t_sd == -1)
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 401 " << nick << " :INVITE :No such nickname\r\n";
        std::string error = oss.str();
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }

    //testing if the invited user is already in the channel invited to
    int in_chan = 0;
    for (size_t i = 0; i < env->channels[chan].clients.size(); i++)
    {
        if (env->channels[chan].clients[i] == t_sd)
        {
            in_chan = 1;
            break;
        }
    }
    if (in_chan)
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 443 " << nick << " :INVITE :Is already  on that channel\r\n";
        std::string error = oss.str();
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }

    //in case of MODE +i of the channel, making sure that the user who
    //is inviting is an admin
    if (cf && env->channels[chan].IsInviteOnly == 1)
    {
        for (int i = 0; i < (int)env->channels[chan].admins.size();i++)
		{
			if (env->channels[chan].admins[i] == client_sd)
				af = 1;
		}
        std::ostringstream oss;
		if (af == 1)
        {
            oss << ":" << serverName << " 341 " << nick << " :INVITE :Sent successfully\r\n";
            env->channels[chan].invited.push_back(t_sd);
            std::string msg = sanitize_message(oss.str());
            send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
            
        }
        else 
        {
            oss << ":" << serverName << " 482 " << nick << " :INVITE :Invite Failed, You're not channel operator" << "\r\n";
            std::string msg = sanitize_message(oss.str());
            send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
            return;    
        }
    }
    else  if (cf && env->channels[chan].IsInviteOnly == -1)
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 341 " << nick << " :INVITE :Sent successfully\r\n"; 
        env->channels[chan].invited.push_back(t_sd);
        std::string msg = sanitize_message(oss.str());
        send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    }
    std::stringstream ss;
        ss << ":" << env->clients[client_sd].nickname
           << "!" << env->clients[client_sd].username
           << "@localhost"
           << " INVITE " << nick
           << " :" << chan << "\n";
        std::string msg = sanitize_message(ss.str());
        send(t_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        std::cout<< msg; 
}
