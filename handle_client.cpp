#include "header.hpp"

void ft_pass(const std::string &buffer, t_environment **env, int client_socket)
{
    std::vector<std::string> a = split_on_space(buffer, ' ');

    if (a.size() != 2)
    {
        std::string error = "Invalid PASS command format.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    if ((*env)->clients[client_socket].pass_flag)
    {
        std::string error = "PASS already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }

    if (a.size() > 1 && a[1] == (*env)->pass)
    {
        (*env)->clients[client_socket].authenticated = true;
        std::string error = "Password accepted.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        std::cout << "Client authenticated successfully." << std::endl;
        (*env)->clients[client_socket].pass_flag = true;
    }
    else
    {
        std::string error = "Incorrect password.\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
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
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    if ((*env)->clients[client_socket].nick_flag)
    {
        std::string error = "NICK already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    
    for (std::map<int, Client>::iterator ip = (*env)->clients.begin(); ip != (*env)->clients.end(); ++ip)
    {
        if (ip->second.nickname == a[1])
        {
            std::string error_msg = "Nickname already taken.\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            std::cout << "Nickname " << a[1] << " is already in use." << std::endl;
            return;
        }
    }
    if (a[1][0] == '#')
    {
        std::string error = "NICK name should not start with #\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    (*env)->clients[client_socket].nickname = a[1];
    
    std::string error = "Nickname accepted.\n";
    error = sanitize_message(error);
    send(client_socket, error.c_str(), error.size(), 0);
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
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    if ((*env)->clients[client_socket].user_flag)
    {
        std::string error = "USER already set!\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
        return;
    }
    for (std::map<int, Client>::iterator ip = (*env)->clients.begin(); ip != (*env)->clients.end(); ++ip)
    {
        if (ip->second.username == a[1])
        {
            std::string error_msg = "Username already taken.\n";
            error_msg = sanitize_message(error_msg);
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
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
            send(client_socket, error.c_str(), error.size(), 0);
        }
        else
        {
            (*env)->clients[client_socket].realname = "NONE";
            std::string error = "Username accepted.\nRealname not accepted\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), 0);
        }
    }
    else
    {
        (*env)->clients[client_socket].realname = "NONE";
        std::string error = "Username accepted.\nRealname not provided\n";
        error = sanitize_message(error);
        send(client_socket, error.c_str(), error.size(), 0);
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
        std::cout<< "hello " << *it << std::endl;
        for (unsigned long i = 0; i < a.size(); i++)
        {
            std::cout<< a[i] <<std::endl;
        }
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
        std::cout<<std::endl<<std::endl<<std::endl<< env->clients[client_socket].nickname<<" helloz " << buffer <<std::endl<<std::endl<<std::endl;
        if (buffer[bytes_received - 1] == '\n')
            buffer[bytes_received - 1] = '\0';
        if (!(env->clients[client_socket].all_set))
            check_which_one_is_flase(buffer, env, client_socket);
        else if ((strncmp(buffer, "JOIN ", 5) == 0) && (env->clients[client_socket].all_set))
            ft_join(client_socket, buffer, env);
        else if ((strncmp(buffer, "PRIVMSG ", 8) == 0) && (env->clients[client_socket].all_set))
            ft_private_message(client_socket, buffer, env);
        else if (!(env->clients[client_socket].all_set))
        {
            std::string error = "You need to register first :D\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), 0);
        }
        else
        {
            std::ostringstream message;
            message << "Command not found: " << buffer << "\n";
            std::string error = message.str();
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), 0);
            std::cout<< env->clients[client_socket].nickname << " send's " << buffer << std::endl;
        }
        // else if ((strncmp(buffer, "TOPIC ", 6) == 0) && (env->clients[client_socket].all_set))
        // {
        //     topic_func(client_socket,buffer,env);
        // }
        // else if ((strncmp(buffer, "INVITE ", 7) == 0) && (env->clients[client_socket].all_set))
        // {
            
        // }
    }
    else if (bytes_received == 0)
    {
        std::cout << "Client disconnected." << std::endl;
        close(client_socket);
        env->clients.erase(client_socket);
    }
    else
    {
        std::cerr << "Error receiving data from client." << std::endl;
    }
}