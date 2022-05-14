#include "Client.hpp"
#include <unistd.h>

Client::Client():
	Socket(0),
	Method(""),
	Path(""),
	Version(""),
	Tmp(""),
	Headers(),
	ResponseStatus(0),
	connected(true),
	full(false),
	FirstRequestLineExist(false)
{}

Client::~Client() {}

void Client::SetSocket(pollfd *socket) {
	this->Socket = socket;
}

void Client::SetConnected(bool connected) {
	this->connected = connected;
}

bool Client::Full() {
	return (this->full);
}

bool Client::Connected() {
	return (this->connected);
}

void Client::Close() {
	::close(this->Socket->fd);
	this->Socket->fd = -1;
}

short Client::Revents() {
	return (this->Socket->revents);
}

int Client::Fd() {
	return (this->Socket->fd);
}