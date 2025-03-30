#include "header.hpp"
static std::string join_tokens(std::vector<std::string>::iterator start, std::vector<std::string>::iterator end)
{
    std::string result;
    for (std::vector<std::string>::iterator it = start; it != end; ++it)
    {
        if (!result.empty())
            result += " ";
        result += *it;
    }
    return result;
}
static std::vector<std::string> split_commas(const std::string &str)
{
    std::vector<std::string> parts;
    std::string tmp;
    for (std::string::size_type i = 0; i < str.size(); ++i)
    {
        if (str[i] == ',')
        {
            if (!tmp.empty())
                parts.push_back(tmp);
            tmp.clear();
        }
        else
            tmp.push_back(str[i]);
    }
    if (!tmp.empty())
        parts.push_back(tmp);
    return parts;
}
void ft_private_message(int client_socket, const std::string &buffer, t_environment *env)
{
    std::vector<std::string> tokens = split_on_space(buffer, ' ');
    if (tokens.size() < 2)
    {
        // Not enough tokens to be a valid PRIVMSG
        std::string err = "PRIVMSG: Not enough parameters.\n";
        send(client_socket, err.c_str(), err.size(), 0);
        return;
    }
    // tokens[0] should be "PRIVMSG"
    // tokens[1] should be <target1,target2,...>
    if (tokens[0] != "PRIVMSG")
    {
        // In a real IRC server, we'd never get here
        // if we only call ft_private_message after checking command == PRIVMSG.
        // But let's be safe:
        std::string err = "Error: This is not a PRIVMSG command.\n";
        send(client_socket, err.c_str(), err.size(), 0);
        return;
    }
    // 2) Extract the list of recipients (the second token).
    std::string rawRecipients = tokens[1]; // e.g. "#chan1,#chan2,userA"
    // 3) The rest is the message. The message might start at tokens[2].
    //    If there's no tokens[2], it means no message text was provided.
    std::string message;
    if (tokens.size() > 2)
    {
        // join everything from tokens[2] onward
        message = join_tokens(tokens.begin() + 2, tokens.end());
        // Typically if it starts with ':', remove that leading colon
        if (!message.empty() && message[0] == ':')
            message.erase(0, 1);
    }
    // 4) Split the rawRecipients by commas to get each recipient
    std::vector<std::string> recipients = split_commas(rawRecipients);
    if (recipients.empty())
    {
        std::string err = "No valid recipients provided.\n";
        send(client_socket, err.c_str(), err.size(), 0);
        return;
    }
    // 5) For each recipient:
    //    - If it starts with '#', treat it as channel
    //    - Else treat it as user
    bool atLeastOneSuccess = false;
    for (std::vector<std::string>::iterator it = recipients.begin();
         it != recipients.end(); ++it)
    {
        // trim leading/trailing spaces just in case
        std::string target = trim_that_first(*it);
        target = trim_that_last(target);
        if (target.empty())
            continue;
        if (target[0] == '#')
        {
            // It's a channel
            //  Check if channel exists
            std::map<std::string, Channel>::iterator chIt = env->channels.find(target);
            if (chIt == env->channels.end())
            {
                // Channel not found
                std::string err = "No such channel: " + target + "\n";
                send(client_socket, err.c_str(), err.size(), 0);
                continue;
            }
            // If you want to enforce “you must be in the channel to speak,” do it here:
            // e.g. check if client_socket is in chIt->second.clients
            // Then broadcast
            std::string broadcastMsg =
                env->clients[client_socket].nickname + " PRIVMSG " +
                target + " :" + message + "\n";
            broadcastMsg = sanitize_message(broadcastMsg);
            // broadcast_message just sends that string to everyone in channel
            broadcast_message(broadcastMsg, target, env);
            atLeastOneSuccess = true;
        }
        else
        {
            // It's a nickname
            bool foundUser = false;
            // Look through env->clients to find who has nickname == target
            for (std::map<int, Client>::iterator cit = env->clients.begin();
                 cit != env->clients.end(); ++cit)
            {
                if (cit->second.nickname == target)
                {
                    // Found the user. Send them the private message
                    std::string privateMsg =
                        env->clients[client_socket].nickname +
                        " PRIVMSG " + target + " :" + message + "\n";
                    privateMsg = sanitize_message(privateMsg);
                    send(cit->first, privateMsg.c_str(), privateMsg.size(), 0);
                    foundUser = true;
                    atLeastOneSuccess = true;
                    break;
                }
            }
            if (!foundUser)
            {
                std::string err = "No such user: " + target + "\n";
                send(client_socket, err.c_str(), err.size(), 0);
            }
        }
    }
    // Optionally, if none of the targets were valid, you can return an error to the sender
    if (!atLeastOneSuccess)
    {
        std::string notice = "PRIVMSG failed: no valid channel/user found.\n";
        send(client_socket, notice.c_str(), notice.size(), 0);
    }
}
