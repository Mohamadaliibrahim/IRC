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


int ft_pass(const std::string &buffer, t_environment **env, int client_socket)
{
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
    std::string msg = get_msg1(buffer);

    for (std::map<int, Client>::iterator it = (*env)->clients.begin(); it != (*env)->clients.end(); ++it)
    {
        if (it->second.nickname == msg)
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
    std::string msg = get_msg1(buffer);

    for (std::map<int, Client>::iterator it = (*env)->clients.begin(); it != (*env)->clients.end(); ++it)
    {
        if (it->second.username == msg)
        {
            std::string error_msg = "Username already taken.\n";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            std::cout << "Username " << msg << " is already in use." << std::endl;
            return;
        }
    }

    (*env)->clients[client_socket].username = msg;
    send(client_socket, "Username accepted.\n", 19, 0);
    std::cout << "Client's username set to: " << msg << std::endl;
    (*env)->clients[client_socket].user_flag = true;
}


void handle_client(int client_socket, t_environment *env)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0)
    {
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
        {
            env->clients[client_socket].all_set = true;
            if ((strncmp(buffer, "JOIN ", 5) == 0) && (env->clients[client_socket].all_set))
            ft_join(client_socket, buffer, env);
            else if ((strncmp(buffer, "PRIVMSG ", 8) == 0) && (env->clients[client_socket].all_set))
                ft_private_message(client_socket, buffer, env);
            else if (!(env->clients[client_socket].all_set))
                send(client_socket, "You need to register first :D\n", 30, 0);
        }
        else
        {
            std::ostringstream message;
            message << "Command not found: " << buffer << "\n";
            send(client_socket, message.str().c_str(), message.str().size(), 0);
            std::cout<< env->clients[client_socket].nickname << " send's " << buffer << std::endl;
        }
        
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