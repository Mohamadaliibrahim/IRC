#include "header.hpp"

int	parse_topic(std::string cmd, std::string &chan, std::string &top, int client_socket, t_environment *env)
{
	std::string serverName = "my.irc.server";
    std::string nick = env->clients[client_socket].nickname;
    std::string user = env->clients[client_socket].username; 
    std::string host = "localhost"; 
	int i = 0;
	if (strncmp(cmd.c_str(), "TOPIC ", 6) == 0 && isspace(cmd[5]))
	{
		i = 6;
		while (isspace(cmd[i]))
			i++;
		if (cmd[i] != '#')
		{
			std::ostringstream oss;
       		oss << ":" << serverName << " 403 " << nick << " :Topic :Channel name must start with a #\r\n";
			std::string error = oss.str();
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
			return -1;
		}
		else
		{
			int j = 0;
			while (!isspace(cmd[i + j]) && cmd[i + j] != '\0')
				j++;
			chan = cmd.substr(i, j);
			if (cmd[i + j] == '\0')
			{
				return 1;
			}
			if (cmd[i + j] != '\0')
			{
				i += j;
				while (isspace(cmd[i]) && cmd[i] != '\0')
					i++;
				if (cmd[i] != ':')
				{
					std::ostringstream oss;
       				oss << ":" << serverName << " 461 " << nick << " :TOPIC :Not enough parameters\r\n";
					std::string error = oss.str();
					error = sanitize_message(error);
					send(client_socket, error.c_str(), error.size(), MSG_NOSIGNAL);
					return -1;
				}
				while (cmd[i] == ':')
					i++;
				while (isspace(cmd[i]) && cmd[i] != '\0')
					i++;
				j = 0;
				while (cmd[i + j] != '\0')
					j++;
				top = cmd.substr(i, j);
				return 2;
			}
		}
	}
	return -1;
}

void	topic_func(int client_sd, std::string cmd, t_environment *env)
{
	std::string chan, top, message;
	std::string serverName = "my.irc.server";
    std::string nick = env->clients[client_sd].nickname;
    std::string user = env->clients[client_sd].username; 
    std::string host = "localhost"; 
	int cf = 0;
	int res = parse_topic(cmd, chan, top, client_sd, env);
	if (res == 2)
	{
		
		if (env->channels.find(chan) != env->channels.end())
		{
			if (env->channels[chan].superUser == client_sd)
			{
				// af = 1;
				std::ostringstream oss;
				oss << ":" << serverName << " 332 " << nick << " :Topic changed to " << top <<"\r\n";
				message = oss.str();
				env->channels[chan].topic = top;
				message = sanitize_message(message);
				send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
				return ;
			}
			for (int i = 0; i < (int)env->channels[chan].clients.size(); i++)
			{
				if (env->channels[chan].clients[i] == client_sd)
				{
					if (env->channels[chan].TopicLock == 1)
					{
						std::ostringstream oss;
						for (int i = 0; i < (int)env->channels[chan].admins.size(); i++)
						{
							if (env->channels[chan].admins[i] == client_sd)
							{
								// af = 1;

								oss << ":" << serverName << " 332 " << nick << " :Topic changed to " << top <<"\r\n";
								env->channels[chan].topic = top;
								message = oss.str();
								message = sanitize_message(message);
								send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
								return ;
							}
						}
						oss << ":" << serverName << " 482 " << nick << " :Topic :cannot edit TOPIC You're not channel operator " << top <<"\r\n";
						message = oss.str();
						message = sanitize_message(message);
						send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
						return ;
					}
					cf = 1;
					std::ostringstream oss;
					oss << ":" << serverName << " 332 " << nick << " :Topic changed to " << top <<"\r\n";
					env->channels[chan].topic = top;
					message = oss.str();
					message = sanitize_message(message);
					send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
				}
			}
			if (cf == 0)
			{
				std::ostringstream oss;
       			oss << ":" << serverName << " 442 " << nick << " :You are not in that channel\r\n";
				std::string error = oss.str();
				error = sanitize_message(error);
				send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
			}
			// else if (cf == 1 && af == 0)
			// {
			// 	std::ostringstream oss;
       		// 	oss << ":" << serverName << " 482 " << nick << " :You're not a channel operator\r\n";
			// 	std::string error = oss.str();
			// 	error = sanitize_message(error);
			// 	send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
			// }
		}
		else
		{
			std::ostringstream oss;
       		oss << ":" << serverName << " 403 " << nick << " :Channel name must start with a #\r\n";
			std::string error = oss.str();
			error = sanitize_message(error);
			send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
		}
	}
	else if (res == 1)
    {
        if (env->channels.find(chan) != env->channels.end())
        {
            for (int i = 0; i < (int)env->channels[chan].clients.size(); i++)
            {
                if (env->channels[chan].clients[i] == client_sd)
                {
                    cf = 1;
                    std::ostringstream oss;
                    if (env->channels[chan].topic.empty())
                        oss << ":" << serverName << " 331 " << nick << " :No topic is set " <<"\r\n"; 
                    else
                        oss << ":" << serverName << " 332 " << nick << " :Topic is : " << env->channels[chan].topic << "\r\n";
                    message = oss.str();
                    message = sanitize_message(message);
                    send(client_sd, message.c_str(), message.size(), MSG_NOSIGNAL);
                }
            }
            if (cf == 0)
            {
                std::ostringstream oss;
                oss << ":" << serverName << " 442 " << nick << " :You are not in that channel\r\n";
                std::string error = oss.str();
                error = sanitize_message(error);
                send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << ":" << serverName << " 403 " << nick << " :Channel name must start with a #\r\n";
            std::string error = oss.str();
            error = sanitize_message(error);
            send(client_sd, error.c_str(), error.size(), MSG_NOSIGNAL);
        }
    }
}

