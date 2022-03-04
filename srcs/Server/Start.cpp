#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Server.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include "webserv.hpp"

static int create_listen_socket(int port)
{
	struct sockaddr_in socket_in;

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
		Server::Log("socket not created");
	}
	Server::Log("Socket create");

	int on = 1;
	if ((setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) { //Разрешаем многократное использование дескриптора сокета
		Server::Log("setsockopt() error");
		close(sock_fd);
	}

	if ((fcntl(sock_fd, F_SETFL, O_NONBLOCK)) < 0) { //Делаем сокет не блокирующим
		Server::Log("fcntl() error");
		close(sock_fd);
	}

	memset(&socket_in, 0, sizeof(socket_in));
	socket_in.sin_family = PF_INET;
	socket_in.sin_port = htons(port); //Задаем порт, который будем слушать
	socket_in.sin_addr.s_addr = inet_addr("0.0.0.0"); //IP

	if (bind(sock_fd, (const struct sockaddr *)&socket_in, sizeof(socket_in)) < 0) { // связываем сокет с именем ??
		Server::Log("bind() error");
		close(sock_fd);
	}

	if ((listen(sock_fd, 1024)) < 0) { //Слушаем сокет
		Server::Log("listen() error");
		close(sock_fd);
	}
	return (sock_fd);
}

static void openConnection(std::vector<pollfd> &fds, int i, std::map<int, std::string> &fd_ip)
{
	struct sockaddr_in in;
	socklen_t len_in = sizeof(in);

	int new_sd = 0;
	while (new_sd != -1)
	{
		new_sd = accept(fds[i].fd, (sockaddr *)&in, &len_in); // тут на счет 1 параметра не уверен до конца
		if (new_sd < 0)
			break;
		pollfd fd;
		fd.fd = new_sd;
		fd.events = POLLIN;
		std::cout << GREEN "New connection with fd: " << new_sd << WHITE << std::endl;
		fds.push_back(fd);

		char bits[100];
		memset(&bits, 0, sizeof(bits));
		inet_ntop(in.sin_family, &in.sin_addr, bits, sizeof(bits));
		std::string ip(bits);
		Server::Log("Client with ip: " + ip + " and fd: " + std::to_string(new_sd) + " connected");
		fd_ip[fds[fds.size() - 1].fd] = ip;
	}
}

static void closeConnection(std::vector<pollfd> &fds, int i, std::map<int, std::string> &fd_ip)
{
		std::string ip = fd_ip[fds[i].fd];
		Server::Log("Client with ip: " + ip + " and fd: " + std::to_string(fds[i].fd) + " disconnect");
		fd_ip.erase(fds[i].fd); // пока не очень понятно зачем, но пусть
		close(fds[i].fd);
		std::vector<pollfd>::iterator it = fds.begin() + i;
		//std::cout << "CLOSE: " << fds[i].fd << " " << it->fd << std::endl;
		//std::cout << "SIZE BEFORE: " << fds.size() << std::endl;
		fds.erase(it);
		//std::cout << "SIZE after: " << fds.size() << std::endl;
		//Server::Log("Client with ip: " + ip + " disconnect");
}

static std::string readRequest(int fd, int &close_connect)
{
	std::string res;
	char buffer[1000];
	memset(buffer, 0, 1000);
	close_connect = 0;
	while (1)
	{
		int rc = recv(fd, buffer, sizeof(buffer), 0);
		if (rc < 0)
			break; //full data read
		if (rc == 0)
		{
			close_connect = 1;
			break;
		}
		res += buffer;
		memset(buffer, 0, 1000);
	}
	return res;
}

void Server::Start() {
	Server::Log("Start Server");
	for (std::vector<int>::iterator begin = this->ports.begin(); begin != this->ports.end(); begin++) //превращаем спаршенные сокеты в открытые порты 
		this->sockets.push_back(create_listen_socket(*begin));
	for (std::vector<int>::iterator begin = this->sockets.begin(); begin != this->sockets.end(); begin++)
	{
		pollfd fd;
		fd.fd = *begin;
		fd.events = POLLIN;
		fds.push_back(fd);
	}

	int close_connect = 0;
	int rpoll = 0;
	while (1) {
		rpoll = poll(fds.data(), fds.size(), -1);
		if (rpoll <= 0) //POLL Error
			continue;
		//std::cout << "fds.size(): " << fds.size() << std::endl;
		unsigned int current_size = fds.size();
		for (unsigned int i = 0; i < current_size; i++){
			if (fds[i].revents == 0)
				continue;
			else if (std::find(this->sockets.begin(), this->sockets.end(), fds[i].fd) != this->sockets.end()) {
				openConnection(fds, i, this->fd_ip);
			} else {
				//читаем запрос
				std::string request = readRequest(fds[i].fd, close_connect); //вероятно для очень больших запросов эта штука не подойдет 
				//
				// Тут в дело вступет HttpRequest (Ralverta)
				//

				//если GET получаем файл
				std::string http_method = "GET";
				if (http_method == "GET")
					this->GET(fds[i].fd, close_connect, request);

				if (close_connect) {
					closeConnection(fds, i, this->fd_ip);
					close_connect = 0;
				}
			}
		}
	}
}