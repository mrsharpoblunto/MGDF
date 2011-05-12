#include "StdAfx.h"

#include <cctype>
#include <boost/algorithm/string.hpp>

#include "MGDFAlias.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

Alias::Alias(void)
{
}

Alias::~Alias(void)
{
}

void Alias::AddAlias(std::string alias,std::string value)
{
	std::transform(alias.begin(), alias.end(), alias.begin(), (int(*)(int)) std::tolower);
	std::transform(value.begin(), value.end(), value.begin(), (int(*)(int)) std::tolower);

	if (!boost::find_first(alias, VFS_ALIAS_SEPARATOR)) {
		_aliases[alias] = value;
	}
	else {
		std::string message = "'";
		message+=VFS_ALIAS_SEPARATOR;
		message+="' character not allowed as part of an alias.";
		throw new EAliasSyntaxException(message);
	}
}

void Alias::RemoveAlias(std::string alias)
{
	std::transform(alias.begin(), alias.end(), alias.begin(), (int(*)(int)) std::tolower);
	stdext::hash_map<std::string,std::string>::iterator iter = _aliases.find(alias);
	if (iter!=_aliases.end()) {
		_aliases.erase(iter);
	}
}

std::string Alias::ResolveAliases(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), (int(*)(int)) std::tolower);
	std::vector<std::string> splitText;
	boost::split(splitText,text,boost::is_any_of(VFS_ALIAS_SEPARATOR));

	if (splitText.size()==1) {
		return text;//no _aliases present in the string
	}
	//there should always be an odd number of splits in the string if the correct number of alias separator characters were used
	else if (splitText.size() % 2 == 1) {
		std::string result = "";
		for (std::vector<std::string>::iterator it = splitText.begin();it!=splitText.end();++it) {
			//if an alias was found, replace it with the alias value in the new string
			if (_aliases.find(*it)!=_aliases.end()) {
				result += _aliases[*it];
			}
			else {
				result += *it;
			}
		}
		return result;
	}
	else {
		std::string message = "Invalid use of '";
		message+=VFS_ALIAS_SEPARATOR;
		message+="' character in text string.";
		throw new EAliasSyntaxException(message);
	}
	return "";
}

}}}
