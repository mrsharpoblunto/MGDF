#include "StdAfx.h"

#include <cctype>//std::tolower
#include "MGDFNameFilter.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace filters {

FilterBase::FilterBase() 
{
	_chainedFilter = NULL;
}

void FilterBase::Dispose()
{
	delete this;
}

IFileFilter *FilterBase::ChainFilter(IFileFilter *filter)
{
	_chainedFilter = filter;
	return filter;
}

bool FilterBase::FilterFile(const wchar_t *file)
{
	std::wstring fileString(file);
	bool result = DoFilterFile(file);
	if (result && _chainedFilter!=NULL) {
		result = _chainedFilter->FilterFile(file);
	}
	return result;
}

}}}}