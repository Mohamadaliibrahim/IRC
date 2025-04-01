#include "header.hpp"

int parse_invite(const std::string &cmd_line, std::string &nickname, std::string &channel, int client_socket)
{
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
        std::string error = "Error: channel name must start with '#'\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return -1;
    }
    return 0;
}
void invite_func(int client_sd, const std::string &cmd, t_environment *env)
{
    std::string nick;
    std::string chan;
    int parseResult = parse_invite(cmd, nick, chan, client_sd);
    if (parseResult == -1)
        return;
    if (env->channels.find(chan) == env->channels.end())
    {
        std::string error = "ERR: No such channel " + chan + "\n";
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), 0);
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
        std::string error = "ERR: You are not on channel " + chan + "\n";
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), 0);
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
        std::string error = "ERR: No such nickname: " + nick + "\n";
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), 0);
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
        std::string error = "ERR_USERONCHANNEL " + nick + " " + chan + "\n";
        error = sanitize_message(error);
        send(client_sd, error.c_str(), error.size(), 0);
        return;
    }
    {
        std::stringstream ss;
        ss << "341 " << nick << " " << chan << "\n";
        std::string msg = sanitize_message(ss.str());
        send(client_sd, msg.c_str(), msg.size(), 0);
    }
    {
        std::stringstream ss;
        ss << ":" << env->clients[client_sd].nickname
           << "!" << env->clients[client_sd].username
           << "@localhost"         // <-- Always use "localhost"
           << " INVITE " << nick
           << " :" << chan << "\n";
        std::string msg = sanitize_message(ss.str());
        send(t_sd, msg.c_str(), msg.size(), 0);
    }
    // 9) Track the invited user
    //    (Now using 'invitedUsers' vector in Channel; just push_back)
    // env->channels[chan].invitedUsers.push_back(t_sd);
}