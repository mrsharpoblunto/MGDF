#include "stdafx.h"
#include "XercesUtils.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

std::string XercesUtils::ToString(const XMLCh *s)
{
	 char *buff = XMLString::transcode(s);
	 std::string temp(buff);
	 XMLString::release(&buff);
	 return temp;
}


}}}}
