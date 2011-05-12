#pragma once

#include <string>
#include <vector>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs {

/**
Creates zip archive handlers
*/
class ZipArchiveHandlerImpl: public DisposeImpl<IArchiveHandler>
{
public:
	ZipArchiveHandlerImpl(ILogger *logger,IErrorHandler *errorHandler);
	virtual ~ZipArchiveHandlerImpl();
	virtual void Dispose();
	virtual bool IsArchive(const char *path) const; 
	virtual IFile *MapArchive(IFile *parent,const char * archiveFile);

private:
	std::vector<std::string> _fileExtensions;
	ILogger *_logger;
	IErrorHandler *_errorHandler;

	/**
	get the extension of a file
	\return the extension (excluding the preceding '.' if possible, otherwise returns "" if no extension could be found
	*/
	std::string GetFileExtension(std::string file) const;
};

MGDF_CORE_DLL IArchiveHandler *CreateZipArchiveHandlerImpl(ILogger *logger,IErrorHandler *errorHandler);

}}}