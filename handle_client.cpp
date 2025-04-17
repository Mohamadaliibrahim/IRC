#include "header/header.hpp"

std::string trim_that_last_with_flag(const std::string& str, char *x)
{
    size_t end = str.find_last_not_of(" \t\r\n");
    if (end == std::string::npos)
        return "";
    if (end != (str.size() - 1))
        (*x) = 'a';
    return str.substr(0, end + 1);
}

void ft_pass(const std::string &buffer, t_environment **env, int client_socket)
{
    std::vector<std::string> a = split_on_space(buffer, ' ');
    if (a.size() != 2)
    {
        std::string error = "Invalid PASS command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    if ((*env)->clients[client_socket].pass_flag)
    {
        std::string error = "PASS already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    if (a.size() > 1 && a[1] == (*env)->pass)
    {
        (*env)->clients[client_socket].authenticated = true;
        std::string error = "Password accepted.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        std::cout << "Client authenticated successfully." << std::endl;
        (*env)->clients[client_socket].pass_flag = true;
    }
    else
    {
        std::string error = "Incorrect password.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        std::cout << "Client failed to authenticate." << std::endl;
    }
}

void ft_nick(const std::string &buffer, t_environment **env, int client_socket)
{
    std::vector<std::string> a = split_on_space(buffer, ' ');
    if (a.size() != 2)
    {
        std::string error = "Invalid NICK command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    if ((*env)->clients[client_socket].nick_flag)
    {
        std::string error = "NICK already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    for (std::map<int, Client>::iterator ip = (*env)->clients.begin(); ip != (*env)->clients.end(); ++ip)
    {
        if (ip->second.nickname == a[1])
        {
            std::string error_msg = "Nickname already taken.\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
            std::cout << "Nickname " << a[1] << " is already in use." << std::endl;
            return;
        }
    }
    if (a[1][0] == '#')
    {
        std::string error = "NICK name should not start with #\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    (*env)->clients[client_socket].nickname = a[1];
    std::string error = "Nickname accepted.\n";
    error = sanitize_message(error);
    send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
    std::cout << "Client's nickname set to: " << a[1] << std::endl;
    (*env)->clients[client_socket].nick_flag = true;
}

void ft_user(const std::string &buffer, t_environment **env, int client_socket)
{
    std::vector<std::string> a = split_on_space(buffer, ' ');
    if (a.size() != 5)
    {
        std::string error = "Invalid USER command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    if ((*env)->clients[client_socket].user_flag)
    {
        std::string error = "USER already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        return;
    }
    for (std::map<int, Client>::iterator ip = (*env)->clients.begin(); ip != (*env)->clients.end(); ++ip)
    {
        if (ip->second.username == a[1])
        {
            std::string error_msg = "Username already taken.\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
            std::cout << "Username " << a[1] << " is already in use." << std::endl;
            return;
        }
    }
    (*env)->clients[client_socket].username = a[1];
    if (a.size() > 2)
    {
        if (a[2] == "0" && a[3] == "*" && a[4][0] == ':')
        {
            (*env)->clients[client_socket].realname = a[4].substr(1);
            std::string error =  "Username accepted.\nRealname accepted\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        }
        else
        {
            (*env)->clients[client_socket].realname = "NONE";
            std::string error = "Username accepted.\nRealname not accepted\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
        }
    }
    else
    {
        (*env)->clients[client_socket].realname = "NONE";
        std::string error = "Username accepted.\nRealname not provided\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
    }
    std::cout << "Client's username set to: " << (*env)->clients[client_socket].username
              << "\nRealname set to: " << (*env)->clients[client_socket].realname << std::endl;
    (*env)->clients[client_socket].user_flag = true;
}

std::vector<std::string> split_on_backspash_n(const std::string &str)
{
    std::vector<std::string> result;
    std::string word;
    std::istringstream stream(str);
    while (std::getline(stream, word))
    {
        word.erase(std::remove(word.begin(), word.end(), '\r'), word.end());
        if (!word.empty()) {
            result.push_back(word);
        }
    }
    return result;
}

std::vector<std::string> split_on_space(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::string word;
    std::istringstream stream(str);
    while (std::getline(stream, word, delimiter))
    {
        if (!word.empty())
            result.push_back(word);
    }
    return result;
}

void trim_start(char* buffer)
{
    size_t start = 0;
    while (buffer[start] == ' ' || buffer[start] == '\t' || buffer[start] == '\r' || buffer[start] == '\n')
    {
        start++;
    }
    size_t length = strlen(buffer);
    for (size_t i = 0; i < length - start; ++i)
    {
        buffer[i] = buffer[start + i];
    }
    buffer[length - start] = '\0';
}

std::string trim_that_first(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    return str.substr(start);
}

void do_buffer(int client_socket, t_environment *env, const std::string &buffer)
{
    std::vector<std::string> x = split_on_backspash_n(buffer);
    std::vector<std::string>::iterator it = x.begin();
    while (it != x.end())
    {
        *it = trim_that_first(*it);
        std::vector<std::string> a = split_on_space(*it, ' ');
        // for (unsigned long i = 0; i < a.size(); i++)
        // {
        //     std::cout<< a[i] <<std::endl;
        // }
        if (a[0] == "PASS" && !env->clients[client_socket].pass_flag)
        {
            ft_pass(*it, &env, client_socket);
        }
        else if (a[0] == "NICK" && !env->clients[client_socket].nick_flag)
        {
            ft_nick(*it, &env, client_socket);
        }
        else if (a[0] == "USER" && !env->clients[client_socket].user_flag)
        {
            ft_user(*it, &env, client_socket);
        }
        it++;
    }
}

void    check_which_one_is_flase(const std::string &buffer, t_environment *env, int client_socket)
{
    do_buffer(client_socket, env, buffer);
    if ((env->clients[client_socket].pass_flag) && (env->clients[client_socket].nick_flag) && (env->clients[client_socket].user_flag))
            env->clients[client_socket].all_set = true;
}

void handle_client(int client_socket, t_environment *env)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    trim_start(buffer);
    if (bytes_received > 0)
    {
        if (buffer[bytes_received - 1] == '\n')
            buffer[bytes_received - 1] = '\0';
        if (!(env->clients[client_socket].all_set))
            check_which_one_is_flase(buffer, env, client_socket);
        else if ((strncmp(buffer, "JOIN ", 5) == 0) && (env->clients[client_socket].all_set))
        {
            ft_join(client_socket, buffer, env);
        }
        else if ((strncmp(buffer, "PRIVMSG ", 8) == 0) && (env->clients[client_socket].all_set))
        {

            ft_private_message(client_socket, buffer, env);
        }
        else if ((strncmp(buffer, "TOPIC ", 6) == 0) && (env->clients[client_socket].all_set))
        {
            char a = '\0';
            std::string temp = (std::string)buffer;
            std::string jnde = trim_that_last_with_flag(buffer, &a);
            topic_func(client_socket,jnde,env);
        }
        else if ((strncmp(buffer, "INVITE ", 7) == 0) && (env->clients[client_socket].all_set))
        {
            char a = '\0';
            std::string temp = (std::string)buffer;
            std::string jnde = trim_that_last_with_flag(buffer, &a);
            invite_func(client_socket,jnde,env);
        }
        else if ((strncmp(buffer, "KICK ", 5) == 0) && (env->clients[client_socket].all_set))
        {
            char a = '\0';
            std::string temp = (std::string)buffer;
            std::string jnde = trim_that_last_with_flag(buffer, &a);
            kick_func(client_socket,jnde,env);
        }
        else if ((strncmp(buffer, "MODE ", 5) == 0) && (env->clients[client_socket].all_set)) // <--- FIX (jnde instead of static var)
        {
            char a = '\0';
            std::string temp = (std::string)buffer;
            std::string jnde = trim_that_last_with_flag(buffer, &a);
            mode_func(client_socket,jnde,env);
        }
        else
        {
        }
    }
    else if (bytes_received == 0)
    {
        std::cout << "Client disconnected." << std::endl;

        int leaving_socket = client_socket;
        std::string leaving_nick = env->clients[leaving_socket].nickname;
        std::string leaving_user = env->clients[leaving_socket].username;
        for (std::map<std::string, Channel>::iterator it = env->channels.begin();
             it != env->channels.end(); )
        {
            Channel &chan = it->second;
            bool isInChannel = false;
            for (size_t i = 0; i < chan.clients.size(); i++)
            {
                if (chan.clients[i] == client_socket)
                {
                    isInChannel = true;
                    break;
                }
            }
            if (!isInChannel)
            {
                continue;
            }
            else 
            {
                std::cout << "here\n";
                std::stringstream ss;
                ss << ":" << leaving_nick
                   << "!" << leaving_user
                   << "@localhost KICK " << chan.name
                   << " " << leaving_nick
                   << ": left the channel" << std::endl; 
                
                std::string kickMsg = sanitize_message(ss.str());
                chan.clients.erase(std::remove(chan.clients.begin(), chan.clients.end(), leaving_socket), chan.clients.end());
                chan.normalUsers.erase(std::remove(chan.normalUsers.begin(), chan.normalUsers.end(), leaving_socket), chan.normalUsers.end()); // <--- FIX (was commented)
                chan.admins.erase(std::remove(chan.admins.begin(), chan.admins.end(), leaving_socket), chan.admins.end());
                
                // Send to remaining users in the channel
                for (size_t i = 0; i < chan.clients.size(); i++)
                {
                    send(chan.clients[i], kickMsg.c_str(), kickMsg.size(), MSG_NOSIGNAL);
                }
                // remove from membership vectors

                // if user was superUser, elect a replacement
                if (chan.superUser == leaving_socket)
                {
                    if (!chan.admins.empty())
                    {
                        chan.superUser = chan.admins.front();
                    }
                    else if (!chan.clients.empty())
                    {
                        chan.superUser = chan.clients.front();
                    }
                    else
                    {
                        chan.superUser = -1;
                    }

                    if (chan.superUser != -1)
                    {
                        std::cout << "New super user for channel " << chan.name
                                  << " is " << env->clients[chan.superUser].nickname << std::endl;
                    }
                }

                // delete empty channels
                if (chan.clients.empty() && chan.normalUsers.empty() && chan.admins.empty())
                {
                    std::cout << "Channel " << chan.name << " is now empty and will be removed." << std::endl;
                    env->channels.erase(it++);
                    continue;                 
                }
                ++it;                         
            }
        }
        close(leaving_socket);
        env->clients.erase(leaving_socket);
        // --- end of enhanced clean‑up
    }
}
