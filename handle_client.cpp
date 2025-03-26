#include "header.hpp"

std::string get_msg1(const std::string &buffer)
{
    // Find the first space after the command
    size_t target_end = buffer.find(" ", 0);
    
    // Check if there was no space (i.e., just the command, no arguments)
    if (target_end == std::string::npos)
        return "";  // If there's no space, there's no message

    // Look for a colon to check if the message starts with ":"
    size_t colon_pos = buffer.find(":", target_end);
    std::string msg;

    if (colon_pos != std::string::npos) // If there's a colon after the space
    {
        msg = buffer.substr(colon_pos + 1);  // Everything after ":"
    }
    else
    {
        // If there's no colon, take everything after the first space
        size_t msg_end = buffer.find(" ", target_end + 1);  // Find next space
        if (msg_end != std::string::npos)
        {
            msg = buffer.substr(target_end + 1, msg_end - target_end - 1);  // Extract the first word
        }
        else
        {
            msg = buffer.substr(target_end + 1);  // If no space, take the rest
        }
    }
    return msg;
}

std::pair<std::string, std::string> get_msg2(const std::string &buffer)
{
    // Find the first space after "USER"
    size_t target_end = buffer.find(" ", 0);
    if (target_end == std::string::npos) return std::pair<std::string, std::string>("", "");  // Return an empty pair

    // Extract the username (between "USER" and first space)
    std::string username = buffer.substr(target_end + 1, buffer.find(" ", target_end + 1) - target_end - 1);

    // Find the colon for realname after "0 *"
    size_t colon_pos = buffer.find(":", target_end);
    std::string realname = "";

    if (colon_pos != std::string::npos) {
        realname = buffer.substr(colon_pos + 1);  // Everything after the ":"
    }

    return std::pair<std::string, std::string>(username, realname);  // Return a pair using constructor
}



int ft_pass(const std::string &buffer, t_environment **env, int client_socket)
{
    std::vector<std::string> x = split(buffer, ' ');
    if (x.size() != 2)
        return (0);
    if ((*env)->clients[client_socket].pass_flag)
    {
        send(client_socket, "PASS already set!\n", 19, 0);
        return (0);
    }
    std::string msg = get_msg1(buffer);
    if (msg == (*env)->pass)
    {
        (*env)->clients[client_socket].authenticated = true;
        send(client_socket, "Password accepted.\n", 19, 0);
        std::cout << "Client authenticated successfully." << std::endl;
        (*env)->clients[client_socket].pass_flag = true;
    }
    else
    {
        send(client_socket, "Incorrect password.\n", 20, 0);
        std::cout << "Client failed to authenticate." << std::endl;
    }
    return (0);
}

void ft_nick(const std::string &buffer, t_environment **env, int client_socket)
{
    if ((*env)->clients[client_socket].nick_flag)
    {
        send(client_socket, "NICK already set!\n", 19, 0);
        return ;
    }
    std::string msg = get_msg1(buffer);

    for (std::map<int, Client>::iterator ip = (*env)->clients.begin(); ip != (*env)->clients.end(); ++ip)
    {
        if (ip->second.nickname == msg)
        {
            std::string error_msg = "Nickname already taken.\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            std::cout << "Nickname " << msg << " is already in use." << std::endl;
            return;
        }
    }

    (*env)->clients[client_socket].nickname = msg;
    send(client_socket, "Nickname accepted.\n", 19, 0);
    std::cout << "Client's nickname set to: " << msg << std::endl;
    (*env)->clients[client_socket].nick_flag = true;
}


void ft_user(const std::string &buffer, t_environment **env, int client_socket)
{
    if ((*env)->clients[client_socket].user_flag)
    {
        send(client_socket, "USER already set!\n", 19, 0);
        return ;
    }
    std::pair<std::string, std::string> user_data = get_msg2(buffer);

    std::string username = user_data.first;
    std::string realname = user_data.second;

    // Check if the username is already taken
    for (std::map<int, Client>::iterator it = (*env)->clients.begin(); it != (*env)->clients.end(); ++it)
    {
        if (it->second.username == username)
        {
            std::string error_msg = "Username already taken.\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            std::cout << "Username " << username << " is already in use." << std::endl;
            return;
        }
    }

    // Assign the username and realname
    (*env)->clients[client_socket].username = username;
    (*env)->clients[client_socket].realname = realname;  // Assuming there's a 'realname' field in the Client struct

    send(client_socket, "Username accepted.\n", 19, 0);
    std::cout << "Client's username set to: " << username << ", Realname set to: " << realname << std::endl;
    (*env)->clients[client_socket].user_flag = true;
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::string word;
    std::istringstream stream(str);

    while (std::getline(stream, word, delimiter)) {
        result.push_back(word);
    }

    return result;
}

void do_buffer(int client_socket, t_environment *env, const std::string &buffer)
{
    std::vector<std::string> x = split(buffer, ' ');  // Split by space, not buffer.size()
    std::vector<std::string>::iterator it = x.begin();
    if (x.size() <= 2)
        return ;
    while (it != x.end())
    {
        
        // Print the position and value of the iterator
        // if (*it == "123")
        //     std::cout << "Found '123' at position: " << position << std::endl;
        
        // std::cout << *it << std::endl;
        if (!(*it).empty() && (*it)[(*it).size() - 1] == '\n') {
            (*it)[(*it).size() - 1] = '\0';
        }
        size_t position = std::distance(x.begin(), it);
        // (void)position;
        std::cout << "Iterator at position " << position << ": " << *it << std::endl;
        if ((*it == "PASS") && (!(env->clients[client_socket].pass_flag)))
        {
            if (env->clients[client_socket].pass_flag)
            {
                send(client_socket, "PASS already set!\n", 19, 0);
                return ;
            }
            if (*(it + 1) == env->pass)
            {
                env->clients[client_socket].authenticated = true;
                send(client_socket, "Password accepted.\n", 19, 0);
                std::cout << "Client authenticated successfully." << std::endl;
                env->clients[client_socket].pass_flag = true;
            }
            else
            {
                send(client_socket, "Incorrect password.\n", 20, 0);
                std::cout << "Client failed to authenticate." << std::endl;
            }
        }
        else if ((*it == "NICK") && (!(env->clients[client_socket].nick_flag)))
        {
            if (env->clients[client_socket].nick_flag)
            {
                send(client_socket, "NICK already set!\n", 19, 0);
                return ;
            }
            for (std::map<int, Client>::iterator ip = env->clients.begin(); ip != env->clients.end(); ++ip)
            {
                if (ip->second.nickname == *(it + 1))
                {
                    std::string error_msg = "Nickname already taken.\n";
                    send(client_socket, error_msg.c_str(), error_msg.size(), 0);
                    std::cout << "Nickname " << *(it + 1) << " is already in use." << std::endl;
                    return;
                }
            }
            env->clients[client_socket].nickname = *(it + 1);
            send(client_socket, "Nickname accepted.\n", 19, 0);
            std::cout << "Client's nickname set to: " << *(it + 1) << std::endl;
            env->clients[client_socket].nick_flag = true;
        }
        else if ((*it == "USER") && (!(env->clients[client_socket].user_flag)))
        {
            if (env->clients[client_socket].user_flag)
            {
                send(client_socket, "USER already set!\n", 19, 0);
                return ;
            }
            if (std::distance(it, x.end()) < 2)
            {
                send(client_socket, "Invalid USER command format.\n", 28, 0);
                return;  // If not enough arguments, return an error
            }
            for (std::map<int, Client>::iterator ip = env->clients.begin(); ip != env->clients.end(); ++ip)
            {
                if (ip->second.username == (*(it + 1)))
                {
                    std::string error_msg = "Username already taken.\n";
                    send(client_socket, error_msg.c_str(), error_msg.size(), 0);
                    std::cout << "Username " << (*(it + 1)) << " is already in use." << std::endl;
                    return;
                }
            }
            env->clients[client_socket].username = (*(it + 1));
            if (std::distance(it, x.end()) > 2)  // Ensure there's at least one more argument for realname
            {
                // Check if the realname format is correct (starts with '0 *:')
                if (*(it + 2) == "0" && *(it + 3) == "*" && *(it + 4) == ":" && !(*(it + 5)).empty())
                {
                    // Realname starts from the 5th element onwards
                    env->clients[client_socket].realname = *(it + 5);
                }
                else
                {
                    // If format is incorrect, set the realname as empty
                    env->clients[client_socket].realname = "NON";
                }
            }
            else
                env->clients[client_socket].realname = "NON";


            send(client_socket, "Username accepted.\n", 19, 0);
            std::cout << "Client's username set to: " << env->clients[client_socket].username << "\nRealname set to: " << env->clients[client_socket].realname << std::endl;
            env->clients[client_socket].user_flag = true;
        }
        it++;
    }
}


void    check_which_one_is_flase(const std::string &buffer, t_environment *env, int client_socket)
{
    do_buffer(client_socket, env, buffer);
}


void handle_client(int client_socket, t_environment *env)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0)
    {
        std::cout<<std::endl<<std::endl<<std::endl<< env->clients[client_socket].nickname<<" helloz " << buffer <<std::endl<<std::endl<<std::endl;
        if (buffer[bytes_received - 1] == '\n')
            buffer[bytes_received - 1] = '\0';
        if (strncmp(buffer, "PASS ", 5) == 0)
        {
            if (ft_pass(buffer, &env, client_socket))
                return ;
        }
        else if (strncmp(buffer, "NICK ", 5) == 0)
            ft_nick(buffer, &env, client_socket);
        else if(strncmp(buffer, "USER ", 5) == 0)
            ft_user(buffer, &env, client_socket);
        else if ((env->clients[client_socket].pass_flag) && (env->clients[client_socket].nick_flag) && (env->clients[client_socket].user_flag))
            env->clients[client_socket].all_set = true;
        if (!(env->clients[client_socket].all_set))
            check_which_one_is_flase(buffer, env, client_socket);
        else if ((strncmp(buffer, "JOIN ", 5) == 0) && (env->clients[client_socket].all_set))
            ft_join(client_socket, buffer, env);
        else if ((strncmp(buffer, "PRIVMSG ", 8) == 0) && (env->clients[client_socket].all_set))
            ft_private_message(client_socket, buffer, env);
        else if (!(env->clients[client_socket].all_set))
            send(client_socket, "You need to register first :D\n", 30, 0);
        else
        {
            std::ostringstream message;
            message << "Command not found: " << buffer << "\n";
            send(client_socket, message.str().c_str(), message.str().size(), 0);
            std::cout<< env->clients[client_socket].nickname << " send's " << buffer << std::endl;
        }
        // else if ((env->clients[client_socket].pass_flag) && (env->clients[client_socket].nick_flag) && (env->clients[client_socket].user_flag))
        // {
            // env->clients[client_socket].all_set = true;
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