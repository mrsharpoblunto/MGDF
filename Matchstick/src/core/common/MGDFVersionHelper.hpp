#pragma once

namespace MGDF { namespace core {

class MGDF_CORE_COMMON_DLL VersionHelper
{
public:
	static Version Create(std::string version);
	static Version Copy(const Version *version);
	static std::string Format(const Version *version);
	static int Compare(const Version *a,const Version *b);
};

}}