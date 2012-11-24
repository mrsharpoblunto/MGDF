#include "StdAfx.h"

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "MGDFVersionHelper.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

std::string VersionHelper::Format(const Version *version)
{
	std::string result = boost::lexical_cast<std::string>(version->Major) + "." + boost::lexical_cast<std::string>(version->Minor);
	
	if (version->Build>=0) {
		result+="."+boost::lexical_cast<std::string>(version->Build);
	}
	if (version->Revision>=0) {
		result+="."+boost::lexical_cast<std::string>(version->Revision);
	}
	return result;
}

Version VersionHelper::Create(const std::string &version)
{
		std::vector<std::string> versionSplit;
		boost::split(versionSplit,version,boost::is_any_of("."));
		Version result;
		result.Major = boost::lexical_cast<int>(versionSplit[0]);
		result.Minor = boost::lexical_cast<int>(versionSplit[1]);
		result.Build = versionSplit.size()>2 ? boost::lexical_cast<int>(versionSplit[2]): -1;
		result.Revision = versionSplit.size()>3 ?boost::lexical_cast<int>(versionSplit[3]): -1;

		return result;
}

Version VersionHelper::Copy(const Version *version)
{
	Version result;
	result.Major = version->Major;
	result.Minor = version->Minor;
	result.Build = version->Build;
	result.Revision = version->Revision;

	return result;
}

INT32 VersionHelper::Compare(const Version *a,const Version *b)
{
	if (a->Major!=b->Major) {
		return a->Major>b->Major?1:-1;
	}
	else if (a->Minor!=b->Minor) {
		return a->Minor>b->Minor?1:-1;
	}
	else if (a->Build!=b->Build) {
		return a->Build>b->Build?1:-1;
	}
	else if (a->Revision!=b->Revision) {
		return a->Revision>b->Revision?1:-1;
	}
	else {
		return 0;
	}
}

}}