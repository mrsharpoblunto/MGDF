#pragma once

#include <string>
#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

/**
Translates common error codes to more readable descriptions
*/
class MGDF_CORE_COMMON_DLL Win32Exception
{
public:
	static std::string TranslateError(unsigned code);
};

}}
