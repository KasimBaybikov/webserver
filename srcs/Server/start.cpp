#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include "Server.hpp"
#include "webserv.hpp"

int compress_array(pollfd *fds, int &nfds)
{
	for (int i = 0; i < nfds; i++)
	{
		if (fds[i].fd == -1)
		{
			for(int j = i; j < nfds-1; j++)
				fds[j].fd = fds[j+1].fd;
			i--;
			nfds--;
		}
	}
	return 1;
}

int create_listen_socket(int port)
{
	struct sockaddr_in socket_in;

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1)
	{
		log("socket not created");
		exit(1);
	}
	log("Socket create");

	int on = 1;
	if ((setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) //Разрешаем многократное использование дескриптора сокета
	{
		log("setsockopt() error");
		close(sock_fd);
		exit(1);
	}

	if ((fcntl(sock_fd, F_SETFL, O_NONBLOCK)) < 0) //Делаем сокет не блокирующим
	{
		log("fcntl() error");
		close(sock_fd);
		exit(1);
	}

	memset(&socket_in, 0, sizeof(socket_in));
	socket_in.sin_family = PF_INET;
	socket_in.sin_port = htons(port); //Задаем порт, который будем слушать
	socket_in.sin_addr.s_addr = inet_addr("0.0.0.0"); //IP

	if (bind(sock_fd, (const struct sockaddr *)&socket_in, sizeof(socket_in)) < 0) // связываем сокет с именем ??
	{
		log("bind() error");
		close(sock_fd);
		exit(1);
	}

	if ((listen(sock_fd, 1024)) < 0) //Слушаем сокет
	{
		log("listen() error");
		close(sock_fd);
		exit(1);
	}
	return (sock_fd);
}

void Server::start()
{
	log("Start Server");
	for (std::vector<int>::iterator begin = this->ports.begin(); begin != this->ports.end(); begin++) //превращаем спаршенные сокеты в открытые порты 
		this->sockets.push_back(create_listen_socket(*begin));
	//pollfd fds[1000];
	memset(fds, 0 , sizeof(fds));
	int nfds = this->sockets.size();
	int i = 0;
	for (std::vector<int>::iterator begin = this->sockets.begin(); begin != this->sockets.end(); begin++)
	{
		fds[i].fd = *begin;
		fds[i].events = POLLIN;
		i++;
	}

	//int close_connect = 0;
	int need_compress_array = 0;
	int rpoll = 0;
	while (1)
	{
		rpoll = poll(fds, nfds, -1);
		if (rpoll <= 0) //POLL Error
			continue;
		unsigned int current_size = nfds;
		for (unsigned int i = 0; i < current_size; i++)
		{
			if (fds[i].revents == 0)
				continue;
			else if (std::find(this->sockets.begin(), this->sockets.end(), fds[i].fd) != this->sockets.end())
				openConnection(fds, nfds, i);
			else
			{
				//читаем запрос
				this->request = readRequest(fds[i].fd, close_connect); //вероятно для очень больших запросов эта штука не подойдет 

				//
				// Тут в дело вступет HttpRequest (Ralverta)
				//

				//если GET получаем файл
				this->http_method = "GET";
				if (this->http_method == "GET")
					this->GET(fds[i].fd);
				this->request.clear();

				if (this->close_connect)
					need_compress_array = closeConnection(close_connect, fds, i);
			}
		}
		if (need_compress_array)
			need_compress_array = compress_array(fds, nfds);
	}
}
