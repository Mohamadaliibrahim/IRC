#include "header/header.hpp"

static std::string join_tokens(std::vector<std::string>::iterator start,
                               std::vector<std::string>::iterator end)
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

static bool is_client_in_channel(int client_socket, const Channel &channel)
{
    for (std::vector<int>::const_iterator it = channel.clients.begin();
         it != channel.clients.end(); ++it)
    {
        if (*it == client_socket)
            return true;
    }
    return false;
}

void ft_private_message(int client_socket, const std::string &buffer, t_environment *env)
{
    std::string serverName = "my.irc.server";
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username; 
    std::string host = "localhost";
    std::ostringstream oss, msg;
    std::string err, message, rawRecipients, target, finalMsg;
    char a;
    bool foundUser, atLeastOneSuccess;
    std::vector<std::string> tokens = split_on_space(buffer, ' ');

    if (tokens.size() < 2)
    {
        oss << ":" << serverName << " 461 " << nick << " PRIVMSG :Not enough parameters\r\n";
        err = oss.str();
        err = sanitize_message(err);
        send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
        return;
    }

    if (tokens[0] != "PRIVMSG")
    {
        oss << ":" << serverName << " 421 " << nick << " " 
            << tokens[0] << " :Unknown command\r\n";
        err = oss.str();
        send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
        return;
    }

    rawRecipients = tokens[1];
    if (tokens.size() > 2)
    {
        a = '\0';
        message = join_tokens(tokens.begin() + 2, tokens.end());
        message = trim_that_last_with_flag(message, &a);
        if (!message.empty() && message[0] == ':' && a == 'a')
            message.erase(0, 1);
    }
    std::vector<std::string> recipients = split_commas(rawRecipients);
    if (recipients.empty())
    {
        oss << ":" << serverName << " 411 " << nick
            << " :No recipient given (PRIVMSG)\r\n";
        err = oss.str();
        err = sanitize_message(err);
        send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
        return;
    }

    atLeastOneSuccess = false;

    for (std::vector<std::string>::iterator it = recipients.begin();
         it != recipients.end(); ++it)
    {
        target = trim_that_first(*it);
        target = trim_that_last(target);

        if (target.empty())
            continue;

        if (target[0] == '#')
        {
            std::map<std::string, Channel>::iterator chIt = env->channels.find(target);
            if (chIt == env->channels.end())
            {
                oss << ":" << serverName << " 403 " << nick 
                    << " " << target << " :No such channel\r\n";
                err = oss.str();
                err = sanitize_message(err);
                send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
                continue;
            }
            if (!is_client_in_channel(client_socket, chIt->second))
            {
                oss << ":" << serverName << " 442 " << nick 
                    << " " << target << " :You're not on that channel\r\n";
                err = oss.str();
                err = sanitize_message(err);
                send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
                continue;
            }
            msg << ":" << nick << "!" << user << "@" << host
                << " PRIVMSG " << target << " :" << message << "\r\n";

            finalMsg = sanitize_message(msg.str());
            broadcast_message(finalMsg, target, env);
            atLeastOneSuccess = true;
        }
        else
        {
            foundUser = false;
            for (std::map<int, Client>::iterator cit = env->clients.begin();
                 cit != env->clients.end(); ++cit)
            {
                if (cit->second.nickname == target)
                {
                    msg << ":" << nick << "!" << user << "@" << host
                        << " PRIVMSG " << target << " :" << message << "\r\n";
                    finalMsg = sanitize_message(msg.str());
                    send(cit->first, finalMsg.c_str(), finalMsg.size(), MSG_NOSIGNAL);
                    foundUser = true;
                    atLeastOneSuccess = true;
                    break;
                }
            }
            if (!foundUser)
            {
                oss << ":" << serverName << " 401 " << nick
                    << " " << target << " :No such nick/channel\r\n";
                err = oss.str();
                send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
            }
        }
    }
    if (!atLeastOneSuccess)
    {
        oss << ":" << serverName << " 442 " << nick << " * "<< ":Error to send!\r\n";
        err = oss.str();
        err = sanitize_message(err);
        send(client_socket, err.c_str(), err.size(), MSG_NOSIGNAL);
    }
}