#include "StdAfx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include <boost/filesystem.hpp>
#include "FakeArchiveHandler.hpp"
#include "FakeFile.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

FakeArchiveHandler::FakeArchiveHandler(MGDF::ILogger *logger,MGDF::IErrorHandler *errorHandler)
{
	_logger = logger;
	_errorHandler = errorHandler;
	_fileExtensions.push_back(".fakearchive");
}

FakeArchiveHandler::~FakeArchiveHandler()
{
}

MGDF::IFile *FakeArchiveHandler::MapArchive(MGDF::IFile *parent,const char * archiveFile) 
{
	std::string physicalFile(archiveFile);
	boost::filesystem::path physicalDirectoryPath(physicalFile,boost::filesystem::native);
	std::string name = physicalDirectoryPath.leaf();
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int)) std::tolower);

	FakeFile *rootFile = new FakeFile(parent,archiveFile,name);

	std::string dataString = "hello world";
	char *data = new char[dataString.size()];
	memcpy(data,dataString.c_str(),dataString.size());

	FakeFile *subFile = new FakeFile(rootFile,"testfile.txt",data,dataString.size());
	rootFile->AddChild(subFile);

	return rootFile;
}

void FakeArchiveHandler::Dispose()
{
	delete this;
}

bool FakeArchiveHandler::IsArchive(const char *path) const 
{
	std::string extension = GetFileExtension(std::string(path));
	for(std::vector<std::string>::const_iterator extIter = _fileExtensions.begin();extIter!=_fileExtensions.end();++extIter) {
		if ((*extIter) == extension) {
			return true;
		}
	}
	return false;
}

std::string FakeArchiveHandler::GetFileExtension(std::string filename) const
{
	std::string::size_type pos = filename.rfind('.',filename.length()-1);
	if (pos != std::string::npos) {
		return filename.substr(pos);
	}
	return "";
}