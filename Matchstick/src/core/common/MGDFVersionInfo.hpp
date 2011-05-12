#pragma once

#include <MGDF/MGDF.hpp>
#include <string>

namespace MGDF { namespace core {

class MGDF_CORE_COMMON_DLL MGDFVersionInfo
{
public:
	virtual ~MGDFVersionInfo(){};
	static const std::string MGDF_VERSION;
	static const int MGDF_INTERFACE_VERSION = 1;
};

}}