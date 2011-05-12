#pragma once

#include <boost/regex.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include "MGDFFilterBase.hpp"

namespace MGDF { namespace core { namespace vfs { namespace filters {

class NameFilter: public FilterBase
{
public:
	/**
	filter by name (can use regular expressions in the name), see
	http://www.boost.org/libs/regex/doc/syntax_perl.html
	for details on the regex syntax
	*/
	NameFilter(std::string nameFilter,bool include);
	virtual ~NameFilter();
protected:
	virtual bool DoFilterFile(std::string file);
private:
	boost::regex *_regex;
	bool _include;
};

}}}}