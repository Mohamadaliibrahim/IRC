#include "header.hpp"

/* --------------------------------------------------
 * Helper to sanitize and send a numeric reply:
 *   :server <code> <yourNick> [<arg>] :<text>
 * -------------------------------------------------- */
static void send_numeric_reply(int client_sd, const std::string &yourNick, const std::string &code, const std::string &arg, const std::string &text)
{
    std::stringstream ss;
    ss << ":server " << code << " " << yourNick;
    if (!arg.empty())
        ss << " " << arg;
    ss << " :" << text << "\n";

    std::string msg = sanitize_message(ss.str());
    send(client_sd, msg.c_str(), msg.size(), 0);
}

int parse_kick(const std::string &cmd_line, std::string &channel, std::string &nickname, std::string &comment, int client_socket, t_environment *env)
{
    std::string cmd = cmd_line;
    int i = 0;

    //same fro gpt idk what is its purpose
    if (!cmd.empty() && cmd[0] == ':')
    {
        while (i < (int)cmd.size() && cmd[i] != ' ')
            i++;
        if (i >= (int)cmd.size())
            return -1;
        cmd.erase(0, i + 1);
        i = 0;
    }
    if (cmd.size() < 5)
    {
        // Not enough to hold "KICK "
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461", // ERR_NEEDMOREPARAMS
                           "KICK",
                           "Not enough parameters");
        return -1;
    }

    // 2) Compare first 4 chars to "KICK"
    if (strncmp(cmd.c_str(), "KICK", 4) != 0)
        return -1;

    // 3) Ensure there's a space after "KICK"
    if (!isspace(cmd[4]))
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461", // ERR_NEEDMOREPARAMS
                           "KICK",
                           "Not enough parameters");
        return -1;
    }

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
    int start = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    channel = cmd.substr(start, i - start);
    if (channel.empty() || channel[0] != '#')
    {
        // 476 ERR_BADCHANMASK
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "476", // ERR_BADCHANMASK
                           channel,
                           "Bad Channel Mask");
        return -1;
    }

    // 5) Skip whitespace
    while (i < (int)cmd.size() && isspace(cmd[i])){
        i++;
	}
    if (i >= (int)cmd.size())
    {
        // No nickname(s) given
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461", // ERR_NEEDMOREPARAMS
                           "KICK",
                           "Not enough parameters");
        return -1;
    }

    // 6) Parse nickname(s). This might contain comma-separated nicks.
    start = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    nickname = cmd.substr(start, i - start);

    // 7) Skip whitespace
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;

    // 8) Optional comment (if present, might be prefixed by ':')
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
    std::string channel;
    std::string nick;    // may hold "nick1,nick2,nick3"
    std::string comment;

    // 1) Parse the command
    int parseResult = parse_kick(cmd, channel, nick, comment, client_sd, env);
    if (parseResult == -1)
        return; // parse_kick already sent numeric errors if needed

    // 2) Check if channel exists
    if (env->channels.find(channel) == env->channels.end())
    {
        // 403 ERR_NOSUCHCHANNEL
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "403", // ERR_NOSUCHCHANNEL
                           channel,
                           "No such channel");
        return;
    }
    Channel &ch = env->channels[channel];

    // 3) Check if the kicker (client_sd) is in the channel
    bool isInChannel = false;
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
        // 442 ERR_NOTONCHANNEL
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "442", // ERR_NOTONCHANNEL
                           channel,
                           "You're not on that channel");
        return;
    }

    // 4) Check if the kicker is superUser or an admin
    if (ch.superUser != client_sd)
    {
        bool isAdmin = false;
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
            // 482 ERR_CHANOPRIVSNEEDED
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "482", // ERR_CHANOPRIVSNEEDED
                               channel,
                               "You're not channel operator");
            return;
        }
    }

    // Split the 'nick' string by commas into individual targets
    std::vector<std::string> nicknames;
    {
        std::stringstream ss(nick);
        std::string temp;
        while (std::getline(ss, temp, ','))
        {
            if (!temp.empty())
                nicknames.push_back(temp);
        }
    }

    // 5) For each nickname, do the KICK logic
    for (size_t idx = 0; idx < nicknames.size(); idx++)
    {
        std::string targetNick = nicknames[idx];

        // Trim leading whitespace - remove chars from front
        while (!targetNick.empty() && isspace(targetNick[0]))
            targetNick.erase(0, 1);

        // Trim trailing whitespace - remove chars from the back
        while (!targetNick.empty() && isspace(targetNick[targetNick.size() - 1]))
            targetNick.erase(targetNick.size() - 1);

        if (targetNick.empty())
            continue; // skip empty

        // 5a) Find target user by nickname
        int target_sd = -1;
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
            // 401 ERR_NOSUCHNICK
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "401", // ERR_NOSUCHNICK
                               targetNick,
                               "No such nick/channel");
            continue;
        }

        // 5b) Check if target user is on the channel
        bool targetInChannel = false;
        size_t targetIndex = 0;
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
            // 441 ERR_USERNOTINCHANNEL
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "441", // ERR_USERNOTINCHANNEL
                               targetNick + " " + channel,
                               "They aren't on that channel");
            continue;
        }

        // 5c) Remove the target user from the channel
        ch.clients.erase(ch.clients.begin() + targetIndex);
        // Also remove from normalUsers if present
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

        // 5d) Notify everyone about the KICK
        std::stringstream ss;
        ss << ":" << env->clients[client_sd].nickname
           << "!" << env->clients[client_sd].username
           << "@localhost KICK " << channel
           << " " << targetNick
           << " :" << (comment.empty() ? "Kicked" : comment) << "\n";

        std::string kickMsg = sanitize_message(ss.str());

        // Send to remaining users in the channel
        for (size_t i = 0; i < ch.clients.size(); i++)
        {
            send(ch.clients[i], kickMsg.c_str(), kickMsg.size(), 0);
        }
        // Optionally send to the kicked user
        if (env->clients.find(target_sd) != env->clients.end())
        {
            send(target_sd, kickMsg.c_str(), kickMsg.size(), 0);
        }
    }
}
