#include "StdAfx.h"

#include <cctype>//std::tolower
#include "ZipFileRoot.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace zip {

ZipFileRoot::ZipFileRoot(const std::wstring &path,ILogger *logger,IErrorHandler *errorHandler): DefaultFileImpl(path,logger,errorHandler)
{
	boost::filesystem::wpath physicalDirectoryPath(path,boost::filesystem::native);
	_archiveName = physicalDirectoryPath.filename().native();
	std::transform(_archiveName.begin(), _archiveName.end(), _archiveName.begin(),::towlower);
}	

ZipFileRoot::~ZipFileRoot()
{
}

bool ZipFileRoot::IsArchive() const
{
	return true;
}

const wchar_t *ZipFileRoot::GetArchiveName() const
{ 
	return _archiveName.c_str(); 
}

}}}}