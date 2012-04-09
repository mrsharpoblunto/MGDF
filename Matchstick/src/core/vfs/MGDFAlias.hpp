#pragma once

#include <exception>
#include <boost/unordered_map.hpp>

namespace MGDF { namespace core { namespace vfs {

class Alias
{
public:
	Alias(void);
	~Alias(void);
	void AddAlias(std::wstring alias,std::wstring value);//throws EAliasSyntaxException
	void RemoveAlias(std::wstring alias);
	std::wstring ResolveAliases(std::wstring text);//throws EAliasSyntaxException
private:
	boost::unordered_map<std::wstring,std::wstring> _aliases;

};

class EAliasSyntaxException: public std::exception {
public:
	EAliasSyntaxException(std::string message): std::exception(message.c_str()){}
	virtual ~EAliasSyntaxException(){}
};

}}}

