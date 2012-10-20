#pragma once

namespace MGDF { namespace core {

class VersionHelper
{
public:
	static Version Create(const std::string &version);
	static Version Copy(const Version *version);
	static std::string Format(const Version *version);
	static int Compare(const Version *a,const Version *b);
};

}}