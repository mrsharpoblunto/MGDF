#pragma once

#include <string>
#include <xercesc/util/XMLString.hpp>

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

class XercesUtils
{
public:
	static std::string ToString(const XMLCh *);
};

}}}}
