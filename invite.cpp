#include "header.hpp"

int parse_invite(const std::string &cmd_line, std::string &nickname, std::string &channel, int client_socket, t_environment *env)
{
    std::string serverName = "my.irc.server";
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username; 
    std::string host = "localhost"; 
    std::string cmd = cmd_line;
    int i = 0;
    //gpt 7atta idk le(ymkn shi 5asso b hex chat to test)
    if (!cmd.empty() && cmd[0] == ':')
    {
        while (i < (int)cmd.size() && cmd[i] != ' ')
            i++;
        if (i >= (int)cmd.size())
            return -1;
        cmd.erase(0, i + 1);
        i = 0;
    }
    //la hon
    if (cmd.size() < 7)
        return -1;
    if (strncmp(cmd.c_str(), "INVITE", 6) != 0)
        return -1;
    if (!isspace(cmd[6]))
        return -1;
    i = 6;
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
        return -1;
    int s = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    nickname = cmd.substr(s, i - s);
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
        return -1;
    if (cmd[i] == ':')
        i++;
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
    int af = 0;
    int parseResult = parse_invite(cmd, nick, chan, client_sd,env);
    if (parseResult == -1)
        return;
    if (env->channels.find(chan) == env->channels.end())
    {
        std::ostringstream oss;
        oss << ":" << serverName << " 403 " << nick << " :INVITE :No such channel " << chan << "\r\n";
        std::string error  = oss.str();
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    int cf = 0;
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
    int t_sd = -1;
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
        }
        else 
        {
            oss << ":" << serverName << " 482 " << nick << " :INVITE :Invite Failed, You're not channel operator" << "\r\n";
        }
        std::string msg = sanitize_message(oss.str());
        send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
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
           << "@localhost"         // <-- Always use "localhost"
           << " INVITE " << nick
           << " :" << chan << "\n";
        std::string msg = sanitize_message(ss.str());
        send(t_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    // 9) Track the invited user
    //    (Now using 'invitedUsers' vector in Channel; just push_back)
    // env->channels[chan].invitedUsers.push_back(t_sd);
}
