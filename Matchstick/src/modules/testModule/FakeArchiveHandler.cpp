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
	_fileExtensions.push_back(L".fakearchive");
}

FakeArchiveHandler::~FakeArchiveHandler()
{
}

MGDF::IFile *FakeArchiveHandler::MapArchive(MGDF::IFile *parent,const wchar_t * archiveFile) 
{
	std::wstring physicalFile(archiveFile);
	boost::filesystem::wpath physicalDirectoryPath(physicalFile,boost::filesystem::native);
	std::wstring name = physicalDirectoryPath.filename().native();
	std::transform(name.begin(), name.end(), name.begin(),::towlower);

	FakeFile *rootFile = new FakeFile(parent,archiveFile,name);

	std::string dataString = "hello world";
	char *data = new char[dataString.size()];
	memcpy(data,dataString.c_str(),dataString.size());

	FakeFile *subFile = new FakeFile(rootFile,L"testfile.txt",data,dataString.size());
	rootFile->AddChild(subFile);

	return rootFile;
}

void FakeArchiveHandler::Dispose()
{
	delete this;
}

bool FakeArchiveHandler::IsArchive(const wchar_t *path) const 
{
	std::wstring extension = GetFileExtension(std::wstring(path));
	for(auto extIter = _fileExtensions.begin();extIter!=_fileExtensions.end();++extIter) {
		if ((*extIter) == extension) {
			return true;
		}
	}
	return false;
}

std::wstring FakeArchiveHandler::GetFileExtension(std::wstring filename) const
{
	std::wstring::size_type pos = filename.rfind('.',filename.length()-1);
	if (pos != std::wstring::npos) {
		return filename.substr(pos);
	}
	return L"";
}