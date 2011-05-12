#include "StdAfx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include "MGDFFolderBaseImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

FolderBaseImpl::FolderBaseImpl(std::string folderName,std::string physicalPath)
{
	std::transform(folderName.begin(), folderName.end(), folderName.begin(), (int(*)(int)) std::tolower);
	_name = folderName;
	_path = physicalPath;
}

FolderBaseImpl::~FolderBaseImpl(void)
{
}

bool FolderBaseImpl::IsOpen() const
{
	return false;
}

bool FolderBaseImpl::OpenFile()
{
	return false;
}

void FolderBaseImpl::CloseFile()
{
}

int FolderBaseImpl::Read(void* buffer,int length)
{
	return 0;
}

void FolderBaseImpl::SetPosition(unsigned long pos)
{
}

unsigned long FolderBaseImpl::GetPosition() const
{
	return 0;
}

bool FolderBaseImpl::EndOfFile() const
{
	return true;
}

unsigned long FolderBaseImpl::GetSize()
{
	return 0;
}

bool FolderBaseImpl::IsFolder() const
{
	return true;
}

bool FolderBaseImpl::IsArchive() const
{
	return false;
}

const char *FolderBaseImpl::GetArchiveName() const
{
	return NULL;
}

const char *FolderBaseImpl::GetPhysicalPath() const
{
	return _path.c_str();
}

const char *FolderBaseImpl::GetName() const
{
	return _name.c_str();
}

}}}