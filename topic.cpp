#include "header.hpp"

int	parse_topic(std::string cmd, std::string &chan, std::string &top, int client_socket)
{
	int i = 0;
	if (strncmp(cmd.c_str(), "TOPIC ", 6) == 0 && isspace(cmd[5]))
	{
		i = 6;
		while (isspace(cmd[i]))
			i++;
		if (cmd[i] != '#')
		{
			std::string error = "IDIOT, channel name must start with a # ?!?!?!?!?!?!?!?\n";
            error = sanitize_message(error);
            send(client_socket, error.c_str(), error.size(), 0);
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
				//if it return 1, that means its a display for the topic
				//so b hal 7ale sar ma3na l channel name l badna nt2akkad enna
				//mawjoude then eza yes we disay the topic
			}
			if (cmd[i + j] != '\0')
			{
				i += j;
				while (isspace(cmd[i]) && cmd[i] != '\0')
					i++;
				if (cmd[i] != ':')
				{
					std::string error = "IDIOT, the topic value must start with ':' ?!?!?!?!?!?!?!?\n";
					error = sanitize_message(error);
					send(client_socket, error.c_str(), error.size(), 0);
					return -1;
				}
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
	int cf = 0, af = 0;
	int res = parse_topic(cmd, chan, top, client_sd);
	if (res == 2)
	{
		if (env->channels.find(chan) != env->channels.end())
		{
			for (int i = 0; i < (int)env->channels[chan].clients.size(); i++)
			{
				if (env->channels[chan].clients[i] == client_sd)
				{
					cf = 1;
					for (int i = 0; i < (int)env->channels[chan].admins.size(); i++)
					{
						if (env->channels[chan].admins[i] == client_sd)
						{
							af = 1;
							message = "Topic changed from " + env->channels[chan].topic + " to " + top + "\n :D\n";
							message = sanitize_message(message);
							send(client_sd, message.c_str(), message.size(), 0);
						}
					}
				}
			}
			if (cf == 0)
			{
				std::string error = "nigga you are not in that channel\n";
				error = sanitize_message(error);
				send(client_sd, error.c_str(), error.size(), 0);
			}
			else if (cf == 1 && af == 0)
			{
				std::string error = "nigga you are not an admin in that shit\n";
				error = sanitize_message(error);
				send(client_sd, error.c_str(), error.size(), 0);
			}
		}
		else
		{
			std::string error = "THERE IS NO CHANNEL WITH THIS NAME?!?!!?\n";
			error = sanitize_message(error);
			send(client_sd, error.c_str(), error.size(), 0);
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
					message = "the topic is: " + env->channels[chan].topic + "\n";
					message = sanitize_message(message);
					send(client_sd, message.c_str(), message.size(), 0);
				}
			}
			if (cf == 0)
			{
				std::string error = "nigga you are not in that channel\n";
				error = sanitize_message(error);
				send(client_sd, error.c_str(), error.size(), 0);
			}
		}
		else
		{
			std::string error = "THERE IS NO CHANNEL WITH THIS NAME?!?!!?\n";
			error = sanitize_message(error);
			send(client_sd, error.c_str(), error.size(), 0);
		}
	}
}
