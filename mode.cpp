#include "header/header.hpp"

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

int parse_mode(const std::string &cmd_line, std::string &channel, std::string &modes, std::vector<std::string> &modeParams, int client_socket, t_environment *env)
{
    std::string cmd = cmd_line;
    int i = 0;

    //making sure that the command is at least "MODE "
    if (cmd.size() < 5)
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461",
                           "MODE",
                           "Not enough parameters here");
        return -1;
    }
    if (strncmp(cmd.c_str(), "MODE", 4) != 0)
        return -1;
    if (!isspace(cmd[4]))
    {
        send_numeric_reply(client_socket,
                           env->clients[client_socket].nickname,
                           "461",
                           "MODE",
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
                           "MODE",
                           "Not enough parameters");
        return -1;
    }
    int start = i;
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
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    if (i >= (int)cmd.size())
    {
        return -10;
    }
    start = i;
    while (i < (int)cmd.size() && !isspace(cmd[i]))
        i++;
    modes = cmd.substr(start, i - start);
    while (i < (int)cmd.size() && isspace(cmd[i]))
        i++;
    while (i < (int)cmd.size())
    {
        while (i < (int)cmd.size() && cmd[i] == ':')
            i++;

        int paramStart = i;
        while (i < (int)cmd.size() && !isspace(cmd[i]))
            i++;
        std::string param = cmd.substr(paramStart, i - paramStart);
        if (!param.empty())
            modeParams.push_back(param);

        while (i < (int)cmd.size() && isspace(cmd[i]))
            i++;
    }
    return 0;
}
// i and t don't need any parameters
// o k abd l need a prarameter


// o + <nick> give operator to the nick if exist and in channel
// o - <nick> take operator privilege from the nick if exist and in channel witht operator privilege
// k + <key/pass> set the key/pass to the channel if it exist and check before if the user is an operator in the channel
// k - remove the key/pass of the channel and check before if the user is an operator in the channel
// l + <number> set the limit of the channel but also check the current number is less or equal to the new limit plus check if the user is an operator in the channel
// l - remove the limit of the channel but also check if the user is an operator in the channel


// i + set the channel on invite only  check if the user is an operator in the channel
// i - remove the restriction of invite only check if the user is an operator in the channel 
// t + set the topic to be only visible to operator only check if the user in an operator in the channel
// t - remove the restriction on the topic check if the user is an operator in the channel

// example of a mode cmd MODE #channel -i
// example of a mode cmd MODE #channel +o <nick>
// example of a mode cmd MODE #channel +k <key>
// example of a mode cmd MODE #channel -k 
// example of a mode cmd MODE #channel -o <nick>


void mode_func(int client_sd, const std::string &cmd, t_environment *env)
{
    std::string channel;
    std::string modes;
    std::vector<std::string> modeParams;
    int parseResult = parse_mode(cmd, channel, modes, modeParams, client_sd, env);
    std::cout << "\n\n the modes param are" << std::endl;
    for (int i = 0; i < (int)modeParams.size(); i++)
        std::cout << modeParams[i]<< std::endl;
    std::cout << "the modes param end here" << std::endl;
    if (parseResult == -1)
        return;

    // 2) Check if channel exists
    if (env->channels.find(channel) == env->channels.end())
    {
        // 403 ERR_NOSUCHCHANNEL
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "403",
                           channel,
                           "No such channel");
        return;
    }
    Channel &ch = env->channels[channel];

    // 3) Check if user is in channel
    bool isInChannel = false;
    for (size_t i = 0; i < env->channels[channel].clients.size(); i++)
    {
        if (env->channels[channel].clients[i] == client_sd)
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
                           "442",
                           channel,
                           "You're not on that channel");
        return;
    }
    if (parseResult == -10)
    {
        std::string str;
        std::stringstream ss;
        std::cout << ch.MembersLimit << std::endl;
        ss << "Invite mode: "  <<(ch.IsInviteOnly == 1 ? "+i" : "-i")
        << "\nKey mode: " << (ch.IsThereAPass == 1 ? "+k" : "-k");
        if (ch.MembersLimit == -1)
        {
          ss << "\nlimit is : " <<  "no limit";
        }
        else 
        {
          ss << "\nlimit is : "  << ch.MembersLimit; 
        }
        ss << "\nTOPIC LOCKED: " << (ch.IsThereAPass == 1 ? "YES\n" : "NO\n");
        std::string msg = sanitize_message(ss.str());
        send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        return ;
    }
    // 4) Check if user is superUser or admin (operator privileges)
    bool isOperator = false;
    if (env->channels[channel].superUser == client_sd)
        isOperator = true;
    else
    {
        for (size_t i = 0; i < env->channels[channel].admins.size(); i++)
        {
            if (env->channels[channel].admins[i] == client_sd)
            {
                isOperator = true;
                break;
            }
        }
    }
    if (!isOperator)
    {
        // 482 ERR_CHANOPRIVSNEEDED
        send_numeric_reply(client_sd,
                           env->clients[client_sd].nickname,
                           "482",
                           channel,
                           "You're not channel operator");
        return;
    }

    // We'll parse each character in `modes`. sign will be '+' or '-'.
    // We'll track which modes actually got applied in a final string for broadcast.
    std::string appliedModes;
    size_t paramIndex = 0;
    char sign = '\0';

    for (size_t m = 0; m < modes.size(); m++)
    {
        char c = modes[m];

        // If we see '+' or '-', that's the new sign
        if (c == '+' || c == '-')
        {
            sign = c;
            continue;
        }

        // If we have not yet encountered + or -, it's invalid usage
        if (sign != '+' && sign != '-')
        {
            // 472 ERR_UNKNOWNMODE
            std::string unknown(1, c);
            send_numeric_reply(client_sd,
                               env->clients[client_sd].nickname,
                               "472",
                               unknown,
                               "is unknown mode char");
            continue;
        }

        std::cout << sign << c << "\n";

        switch (c)
        {
            // +i / -i => invite-only channel
            case 'i':
            {
                if (sign == '+')
                    env->channels[channel].IsInviteOnly = 1;   // 1 means "on"
                else
                {
                    env->channels[channel].IsInviteOnly = -1;
                }
                appliedModes.push_back(sign);
                appliedModes.push_back('i');
                break;
            }

            // +t / -t => topic lock
            case 't':
            {
                if (sign == '+')
                    env->channels[channel].TopicLock = 1;
                else
                {
                    env->channels[channel].TopicLock = -1;
                }
                appliedModes.push_back(sign);
                appliedModes.push_back('t');
                break;
            }

            // +k / -k => channel password
            case 'k':
            {
                if (sign == '+')
                {
                    if (paramIndex >= modeParams.size())
                    {
                        send_numeric_reply(client_sd,
                                           env->clients[client_sd].nickname,
                                           "461", // ERR_NEEDMOREPARAMS
                                           "MODE",
                                           "Not enough parameters for +k");
                        break;
                    }
                    env->channels[channel].IsThereAPass = 1;
                    env->channels[channel].pass = modeParams[paramIndex++];
                    appliedModes.push_back('+');
                    appliedModes.push_back('k');
                }
                else
                {
                    // Remove the password
                    env->channels[channel].IsThereAPass = -1;
                    env->channels[channel].pass.clear();
                    appliedModes.push_back('-');
                    appliedModes.push_back('k');
                }
                break;
            }

            // +l / -l => channel user limit
            case 'l':
            {
                if (sign == '+')
                {
                    if (paramIndex >= modeParams.size())
                    {
                        send_numeric_reply(client_sd,
                                           env->clients[client_sd].nickname,
                                           "461",
                                           "MODE",
                                           "Not enough parameters for +l");
                        break;
                    }
                    int limitVal = atoi(modeParams[paramIndex++].c_str());
                    if (limitVal < 0 && limitVal != -1)
                        send_numeric_reply(client_sd,
                                            env->clients[client_sd].nickname,
                                            "472",
                                            "MODE",
                                            "is unknown mode char");
                    else 
                    {
                        env->channels[channel].MembersLimit = limitVal; // set the new limit
                        appliedModes.push_back('+');
                        appliedModes.push_back('l');
                    }
                }
                else
                {
                    // -l => unlimited
                    env->channels[channel].MembersLimit = -1; 
                    appliedModes.push_back('-');
                    appliedModes.push_back('l');
                }
                break;
            }

            // +o / -o => give or remove operator privileges
            case 'o':
            {
                if (paramIndex >= modeParams.size())
                {
                    send_numeric_reply(client_sd,
                                       env->clients[client_sd].nickname,
                                       "461",
                                       "MODE",
                                       "Not enough parameters for +o/-o");
                    break;
                }
                std::string targetNick = modeParams[paramIndex++];
            
                /* 1) locate the target socket */
                int target_sd = -1;
                for (std::map<int, Client>::iterator it = env->clients.begin();
                     it != env->clients.end(); ++it)
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
                    break;
                }
            
                /* 2) make sure the target is on the channel */
                bool targetOnChannel = false;
                for (size_t i = 0; i < ch.clients.size(); ++i)
                {
                    if (ch.clients[i] == target_sd)
                    {
                        targetOnChannel = true;
                        break;
                    }
                }
                if (!targetOnChannel)
                {
                    send_numeric_reply(client_sd,
                                       env->clients[client_sd].nickname,
                                       "441",
                                       targetNick + " " + channel,
                                       "They aren't on that channel");
                    break;
                }
            
                /* 3) handle +o */
                if (sign == '+')
                {
                    bool alreadyAdmin = false;
                    for (size_t i = 0; i < ch.admins.size(); ++i)
                    {
                        if (ch.admins[i] == target_sd)
                        {
                            alreadyAdmin = true;
                            break;
                        }
                    }
                    if (!alreadyAdmin)
                        ch.admins.push_back(target_sd);
                
                    appliedModes += "+o";
                    break;
                }
            
                /* 4) handle -o (removal) */
                /* --- new rules start here ----------------------------------------- */
                /* founder can never be stripped; only founder can strip others       */
                if (target_sd == ch.superUser)
                {
                    send_numeric_reply(client_sd,
                                       env->clients[client_sd].nickname,
                                       "482",
                                       channel,
                                       "Cannot remove operator status from channel founder");
                    break;
                }
                /* if the requester is NOT the founder and is trying to strip
                   someone other than himself, deny it                               */
                if (client_sd != ch.superUser && client_sd != target_sd)
                {
                    send_numeric_reply(client_sd,
                                       env->clients[client_sd].nickname,
                                       "482",
                                       channel,
                                       "You can only remove your own operator status");
                    break;
                }
                /* --- new rules end here ------------------------------------------- */
            
                /* actually remove */
                for (std::vector<int>::iterator it = ch.admins.begin();
                     it != ch.admins.end(); ++it)
                {
                    if (*it == target_sd)
                    {
                        ch.admins.erase(it);
                        break;
                    }
                }
                appliedModes += "-o";
                break;
            }

            // Anything else is unknown
            default:
            {
                std::string unknown(1, c);
                send_numeric_reply(client_sd,
                                   env->clients[client_sd].nickname,
                                   "472",
                                   unknown,
                                   "is unknown mode char");
                break;
            }
        }
    } // end for

    if (modes.empty())
    {
        std::stringstream ss;
        ss << "Invite mode: "  <<(ch.IsInviteOnly == 1 ? "+i" : "-i")
        << " Key mode: " << (ch.IsThereAPass == 1 ? "+k" : "-k")
        << " limit is : " << (ch.MembersLimit == -1 ? "no limit" : ""+ch.MembersLimit)
        << " TOPIC LOCKED: " << (ch.IsThereAPass == 1 ? "YES" : "NO");
        std::string msg = sanitize_message(ss.str());
        send(client_sd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        return ;
    }
    // If nothing was applied, we can just return
    if (appliedModes.empty())
        return;

    // 5) Broadcast the final mode change to the channel
    {
        // Build the mode line to broadcast
        std::stringstream ss;
        ss << ":" << env->clients[client_sd].nickname
           << "!" << env->clients[client_sd].username
           << "@localhost MODE " << channel << " " << appliedModes << " ";

        // If you want to reflect the actual parameters used for each mode:
        // but for simplicity, let's just append all modeParams
        for (size_t p = 0; p < modeParams.size(); p++)
            ss << modeParams[p] << " ";
        ss << "\n";

        std::string modeMsg = sanitize_message(ss.str());

        // Send to everyone in the channel
        for (size_t i = 0; i < ch.clients.size(); i++)
        {
            send(ch.clients[i], modeMsg.c_str(), modeMsg.size(), MSG_NOSIGNAL);
        }
    }
}

