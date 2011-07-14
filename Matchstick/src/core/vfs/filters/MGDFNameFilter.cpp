#include "StdAfx.h"

#include <cctype>//std::tolower
#include "MGDFNameFilter.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace filters {

NameFilter::NameFilter(std::wstring nameFilter,bool include)
{
	std::transform(nameFilter.begin(), nameFilter.end(), nameFilter.begin(),::towlower);
	_regex = new boost::wregex(nameFilter);
	this->_include = include;
}

NameFilter::~NameFilter()
{
	delete _regex;
}

bool NameFilter::DoFilterFile(std::wstring file)
{
	std::transform(file.begin(), file.end(), file.begin(), ::towlower);
	if (boost::regex_match(file.begin(),file.end(),*_regex)) {
		return _include;
	}
	else {
		return !_include;
	}
}

}}}}