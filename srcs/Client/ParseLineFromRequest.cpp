#include "Client.hpp"

void Client::ParseLineFromRequest(std::string line) {
	//std::cout << "ParseLineFromRequest: " << line << std::endl;
	if (!this->FirstRequestLineExist) {
		FirstRequestLine(line);
	}
	//std::cout << this->ResponseStatus << std::endl;
			//AddHeader(line);
}