#pragma once

#include <MGDF/MGDFVirtualFileSystem.hpp>
#include "MGDFNameFilter.hpp"

namespace MGDF { namespace core { namespace vfs { namespace filters {

class ExtensionFilter: public NameFilter
{
public:
	ExtensionFilter(const std::wstring &extensionFilter,bool include);
	virtual ~ExtensionFilter();
};

}}}}