#include "header/header.hpp"

//send the messages to the server
static void send_numeric_reply(int client_sd, const std::string &yourNick, const std::string &code, const std::string &arg, const std::string &text)
{
    std::stringstream ss;

    ss << ":server " << code << " " << yourNick;
    if (!arg.empty())
        ss << " " << arg;
    ss << " :" << text << "\n";

    std::string msg = sanitize_message(ss.str());
    send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

int parse_kick(const std::string &cmd_line, std::string &channel, std::string &nickname, std::string &comment, int client_socket, t_environment *env)
{
    std::string cmd = cmd_line;
    int i = 0, start;

    //making sure that the command is at least "KICK "
    if (cmd.size() < 5)
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461",
                           "KICK",
                           "Not enough parameters");
        return -1;
    }
    if (strncmp(cmd.c_str(), "KICK", 4) != 0)
        return -1;
    if (!isspace(cmd[4]))
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461", // ERR_NEEDMOREPARAMS
                           "KICK",
                           "Not enough parameters");
        return -1;
    }

    //parsing the parameters after "KICK " with validation
    i = 4;
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461",
                           "KICK",
                           "Not enough parameters");
        return -1;
    }

    // 4) Parse channel (must start with '#')
    start = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    channel = cmd.substr(start, i - start);
    if (channel.empty() || channel[0] != '#')
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "476",
                           channel,
                           "Bad Channel Mask");
        return -1;
    }
    while (i < (int)cmd.size() && isspace(cmd[i])){
        i++;
	}
    if (i >= (int)cmd.size())
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461",
                           "KICK",
                           "Not enough parameters");
        return -1;
    }
    start = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    nickname = cmd.substr(start, i - start);

    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;

    if (i < (int)cmd.size())
    {
        while (cmd[i] == ':')
            i++;
        comment = cmd.substr(i);
    }
    else
        comment.clear();

    return 0;
}

void kick_func(int client_sd, const std::string &cmd, t_environment *env)
{
    std::string channel, comment;
    std::string nick = env->clients[client_sd].nickname;
    std::string user = env->clients[client_sd].username;
    std::string serverName = "my.irc.server";
    std::string message, targetNick, temp;
    std::stringstream ss;
    std::ostringstream oss;
    bool isInChannel, isAdmin, targetInChannel;
    size_t targetIndex;

    //Parse the command
    int parseResult = parse_kick(cmd, channel, nick, comment, client_sd, env);
    int target_sd = -1;
    if (parseResult == -1)
        return; // parse_kick already sent numeric errors if needed

    //Check if channel exists
    if (env->channels.find(channel) == env->channels.end())
    {
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "403",
                           channel,
                           "No such channel");
        return;
    }
    Channel &ch = env->channels[channel];

    //Check if the kicker (client_sd) is in the channel
    isInChannel = false;
    for (size_t i = 0; i < ch.clients.size(); i++)
    {
        if (ch.clients[i] == client_sd)
        {
            isInChannel = true;
            break;
        }
    }
    if (!isInChannel)
    {
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "442",
                           channel,
                           "You're not on that channel");
        return;
    }

    //Check if the kicker is superUser or an admin
    if (ch.superUser != client_sd)
    {
        isAdmin = false;
        for (size_t i = 0; i < ch.admins.size(); i++)
        {
            if (ch.admins[i] == client_sd)
            {
                isAdmin = true;
                break;
            }
        }
        if (!isAdmin)
        {
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "482",
                               channel,
                               "You're not channel operator");
            return;
        }
    }

    //Split the 'nick' string by commas into individual targets
    std::vector<std::string> nicknames;
    {
        std::stringstream ss(nick);
        while (std::getline(ss, temp, ','))
        {
            if (!temp.empty())
                nicknames.push_back(temp);
        }
    }

    //For each nickname, do the KICK logic
    for (size_t idx = 0; idx < nicknames.size(); idx++)
    {
        targetNick = nicknames[idx];

        if (env->clients[client_sd].nickname == targetNick)
        {
            oss << ":" << serverName << " " << nick << " " << channel << " :KICK you can't kick yourself\r\n";
            message = oss.str();
            message = sanitize_message(message);
            send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
            continue ;
        }
        while (!targetNick.empty() && isspace(targetNick[0]))
            targetNick.erase(0, 1);
        while (!targetNick.empty() && isspace(targetNick[targetNick.size() - 1]))
            targetNick.erase(targetNick.size() - 1);

        if (targetNick.empty())
            continue;

        target_sd = -1;
        std::map<int, Client>::iterator it;
        for (it = env->clients.begin(); it != env->clients.end(); ++it)
        {
            if (it->second.nickname == targetNick)
            {
                target_sd = it->first;
                break;
            }
        }
        if (target_sd == -1)
        {
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "401",
                               targetNick,
                               "No such nick/channel");
            continue;
        }

        //Check if target user is on the channel
        targetInChannel = false;
        targetIndex = 0;
        for (size_t i = 0; i < ch.clients.size(); i++)
        {
            if (ch.clients[i] == target_sd)
            {
                targetInChannel = true;
                targetIndex = i;
                break;
            }
        }
        if (!targetInChannel)
        {
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "441",
                               targetNick + " " + channel,
                               "They aren't on that channel");
            continue;
        }

        if (target_sd == ch.superUser && client_sd != ch.superUser)
        {
            send_numeric_reply(client_sd, nick, "482", channel,
                               "Cannot kick channel founder");
            continue;
        }

        //Remove the target user from the channel
        ch.clients.erase(ch.clients.begin() + targetIndex);
        //Also remove from normalUsers if present
        for (std::vector<int>::iterator nIt = ch.normalUsers.begin(); nIt != ch.normalUsers.end(); ++nIt)
        {
            if (*nIt == target_sd)
            {
                ch.normalUsers.erase(nIt);
                break;
            }
        }
        // Also remove from admins if present
        for (std::vector<int>::iterator aIt = ch.admins.begin(); aIt != ch.admins.end(); ++aIt)
        {
            if (*aIt == target_sd)
            {
                ch.admins.erase(aIt);
                break;
            }
        }

        //Notify everyone about the KICK
        ss << ":" << env->clients[client_sd].nickname
           << "!" << env->clients[client_sd].username
           << "@localhost KICK " << channel
           << " " << targetNick
           << " :" << (comment.empty() ? "Kicked" : comment) << "\n";

        message = sanitize_message(ss.str());

        //Send to remaining users in the channel
        for (size_t i = 0; i < ch.clients.size(); i++)
        {
            send(ch.clients[i], message.c_str(), message.size(), MSG_NOSIGNAL);
        }
        //send to the kicked user
        if (env->clients.find(target_sd) != env->clients.end())
        {
            send(target_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
        }
    }
}
