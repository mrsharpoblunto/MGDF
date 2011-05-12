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
	_fileExtensions.push_back(".zip");
	_fileExtensions.push_back(".mza");
}

ZipArchiveHandlerImpl::~ZipArchiveHandlerImpl()
{
}

IFile *ZipArchiveHandlerImpl::MapArchive(IFile *parent,const char * archiveFile) 
{
	zip::ZipArchive *archive = new zip::ZipArchive(_logger,_errorHandler);
	IFile *result = archive->MapArchive(parent,archiveFile);
	if (result==NULL)
	{
		delete archive;
	}
	return result;
}

void ZipArchiveHandlerImpl::Dispose()
{
	delete this;
}

bool ZipArchiveHandlerImpl::IsArchive(const char *path) const 
{
	std::string extension = GetFileExtension(std::string(path));
	for(std::vector<std::string>::const_iterator extIter = _fileExtensions.begin();extIter!=_fileExtensions.end();++extIter) {
		if ((*extIter) == extension) {
			return true;
		}
	}
	return false;
}

std::string ZipArchiveHandlerImpl::GetFileExtension(std::string filename) const
{
	std::string::size_type pos = filename.rfind('.',filename.length()-1);
	if (pos != std::string::npos) {
		return filename.substr(pos);
	}
	return "";
}

}}}