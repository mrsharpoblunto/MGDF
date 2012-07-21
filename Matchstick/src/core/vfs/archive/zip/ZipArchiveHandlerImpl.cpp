#include "StdAfx.h"

#include "ZipArchiveHandlerImpl.hpp"
#include "ZipArchive.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { 

MGDF_CORE_DLL IArchiveHandler *CreateZipArchiveHandlerImpl(ILogger *logger,IErrorHandler *errorHandler)
{
	return new ZipArchiveHandlerImpl(logger,errorHandler);
}

ZipArchiveHandlerImpl::ZipArchiveHandlerImpl(ILogger *logger,IErrorHandler *errorHandler)
{
	_logger = logger;
	_errorHandler = errorHandler;
	_fileExtensions.push_back(L".zip");
}

ZipArchiveHandlerImpl::~ZipArchiveHandlerImpl()
{
}

IFile *ZipArchiveHandlerImpl::MapArchive(IFile *parent,const wchar_t * archiveFile) 
{
	zip::ZipArchive *archive = new zip::ZipArchive(_logger,_errorHandler);
	IFile *result = archive->MapArchive(parent,archiveFile);
	if (result==nullptr)
	{
		delete archive;
	}
	return result;
}

void ZipArchiveHandlerImpl::Dispose()
{
	delete this;
}

bool ZipArchiveHandlerImpl::IsArchive(const wchar_t *path) const 
{
	std::wstring extension = GetFileExtension(std::wstring(path));
	for(auto extIter = _fileExtensions.begin();extIter!=_fileExtensions.end();++extIter) {
		if ((*extIter) == extension) {
			return true;
		}
	}
	return false;
}

std::wstring ZipArchiveHandlerImpl::GetFileExtension(const std::wstring &filename) const
{
	std::wstring::size_type pos = filename.rfind('.',filename.length()-1);
	if (pos != std::wstring::npos) {
		return filename.substr(pos);
	}
	return L"";
}

}}}