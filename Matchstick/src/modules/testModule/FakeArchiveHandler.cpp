#include "StdAfx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include <boost/filesystem.hpp>
#include "FakeArchiveHandler.hpp"
#include "FakeFile.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

static const wchar_t *FAKE_EXT = L".fakearchive";

FakeArchiveHandler::FakeArchiveHandler(MGDF::ILogger *logger,MGDF::IErrorHandler *errorHandler)
{
	_logger = logger;
	_errorHandler = errorHandler;
	_fileExtensions.push_back(FAKE_EXT);
}

FakeArchiveHandler::~FakeArchiveHandler()
{
}

MGDF::IFile *FakeArchiveHandler::MapArchive(const wchar_t *name,const wchar_t * archiveFile,MGDF::IFile *parent)
{
	FakeFile *rootFile = new FakeFile(name,archiveFile,parent);

	std::string dataString = "hello world";
	char *data = new char[dataString.size()];
	memcpy(data,dataString.c_str(),dataString.size());

	FakeFile *subFile = new FakeFile(L"testfile.txt",rootFile,data,dataString.size());
	rootFile->AddChild(subFile);

	return rootFile;
}

void FakeArchiveHandler::Dispose()
{
	delete this;
}

void FakeArchiveHandler::DisposeArchive(MGDF::IFile *file)
{
	delete (FakeFile *)file;
}

bool FakeArchiveHandler::IsArchive(const wchar_t *path) const 
{
	std::wstring extension = GetFileExtension(path);
	for(auto extIter = _fileExtensions.begin();extIter!=_fileExtensions.end();++extIter) {
		if ((*extIter) == extension) {
			return true;
		}
	}
	return false;
}

const wchar_t *FakeArchiveHandler::GetFileExtension(const wchar_t *filename) const
{
	_ASSERTE( filename );
	size_t index = wcslen(filename);
	while (index >= 0) {
		if (filename[index] == '.') return filename + (index*sizeof(wchar_t));
	}
	return nullptr;
}