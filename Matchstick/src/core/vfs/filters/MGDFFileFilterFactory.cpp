#include "StdAfx.h"

#include "MGDFFileFilterFactory.hpp"
#include "MGDFNameFilter.hpp"
#include "MGDFExtensionFilter.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace filters {

IFileFilter *FileFilterFactory::CreateNameExclusionFilter(const wchar_t * name) const
{
	return new NameFilter(std::wstring(name),false);
}

IFileFilter *FileFilterFactory::CreateNameInclusionFilter(const wchar_t * name) const
{
	return new NameFilter(std::wstring(name),true);
}

IFileFilter *FileFilterFactory::CreateFileExtensionExclusionFilter(const wchar_t * extension) const
{
	return new ExtensionFilter(std::wstring(extension),false);
}

IFileFilter *FileFilterFactory::CreateFileExtensionInclusionFilter(const wchar_t * extension) const
{
	return new ExtensionFilter(std::wstring(extension),true); 
}

}}}}
 
