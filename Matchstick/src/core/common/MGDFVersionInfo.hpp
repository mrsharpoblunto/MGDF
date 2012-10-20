#pragma once

#include <MGDF/MGDF.hpp>
#include <string>

namespace MGDF { namespace core {

class MGDFVersionInfo
{
public:
	virtual ~MGDFVersionInfo(){};
	static const std::string &MGDF_VERSION() { return _mgdfVersion; }
	static const int MGDF_INTERFACE_VERSION = 1;
private:
	#pragma warning(push)
	#pragma warning(disable: 4251)
	static const std::string _mgdfVersion;
	#pragma warning(pop)
};

}}