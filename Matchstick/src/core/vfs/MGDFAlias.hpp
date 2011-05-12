#pragma once

#include <exception>
#include <hash_map>

namespace MGDF { namespace core { namespace vfs {

class Alias
{
public:
	Alias(void);
	~Alias(void);
	void AddAlias(std::string alias,std::string value);//throws EAliasSyntaxException
	void RemoveAlias(std::string alias);
	std::string ResolveAliases(std::string text);//throws EAliasSyntaxException
private:
	stdext::hash_map<std::string,std::string> _aliases;

};

class EAliasSyntaxException: public std::exception {
public:
	EAliasSyntaxException(std::string message): std::exception(message.c_str()){}
	virtual ~EAliasSyntaxException(){}
};

}}}

