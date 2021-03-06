#include "Parser.hpp"

void CgiExtensions(Location &location, std::string line) {
	Parser::replace_all(line, "\t", " ");
	std::vector<std::string> directive = Parser::split(line, " ");
	if (directive.size() < 2) {
		throw Parser::InvalidNumberOfArgument();
	}
	if (directive[0] != "cgi_extensions") {
		throw Parser::UnknownDirective(directive[0]);
	}
	if (location.cgi_extensionsExist) {
		throw Parser::DirectiveDuplicate(directive[0]);
	}
	for (uint i = 1; i < directive.size(); i++) {
		if (directive[i][0] != '.')
			throw Parser::UnknownDirective(directive[i]);
	}
	for (uint i = 1; i < directive.size(); i++) {
		location.cgi_extensions.push_back(directive[i]);
	}
	location.cgi_extensionsExist = 1;
}
