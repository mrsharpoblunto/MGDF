#include "StdAfx.h"

#include <cctype>
#include <boost/algorithm/string.hpp>
#include "../common/MGDFResources.hpp"

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

void Alias::AddAlias(std::wstring alias,std::wstring value)
{
	std::transform(alias.begin(), alias.end(), alias.begin(),::towlower);
	std::transform(value.begin(), value.end(), value.begin(),::towlower);

	if (!boost::find_first(alias, VFS_ALIAS_SEPARATOR)) {
		_aliases[alias] = value;
	}
	else {
		std::string message = "'";
		message+=Resources::ToString(VFS_ALIAS_SEPARATOR);
		message+="' character not allowed as part of an alias.";
		throw new EAliasSyntaxException(message);
	}
}

void Alias::RemoveAlias(std::wstring alias)
{
	std::transform(alias.begin(), alias.end(), alias.begin(), ::towlower);
	stdext::hash_map<std::wstring,std::wstring>::iterator iter = _aliases.find(alias);
	if (iter!=_aliases.end()) {
		_aliases.erase(iter);
	}
}

std::wstring Alias::ResolveAliases(std::wstring text)
{
	std::transform(text.begin(), text.end(), text.begin(), ::towlower);
	std::vector<std::wstring> splitText;
	boost::split(splitText,text,boost::is_any_of(VFS_ALIAS_SEPARATOR));

	if (splitText.size()==1) {
		return text;//no _aliases present in the string
	}
	//there should always be an odd number of splits in the string if the correct number of alias separator characters were used
	else if (splitText.size() % 2 == 1) {
		std::wstring result;
		for (std::vector<std::wstring>::iterator it = splitText.begin();it!=splitText.end();++it) {
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
		message+=Resources::ToString(VFS_ALIAS_SEPARATOR);
		message+="' character in text string.";
		throw new EAliasSyntaxException(message);
	}
	return L"";
}

}}}
