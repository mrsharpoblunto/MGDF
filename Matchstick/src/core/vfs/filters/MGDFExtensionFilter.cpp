#include "StdAfx.h"
#include "MGDFExtensionFilter.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace filters {

ExtensionFilter::ExtensionFilter(const std::wstring &extension,bool include):NameFilter(L"^.+\\.("+extension+L")$",include) 
{
}

ExtensionFilter::~ExtensionFilter(){}

}}}}