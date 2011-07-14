#pragma once

#include <string>
#include <vector>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

class FakeArchiveHandler: public MGDF::DisposeImpl<MGDF::IArchiveHandler>
{
public:
	FakeArchiveHandler(MGDF::ILogger *logger,MGDF::IErrorHandler *errorHandler);
	virtual ~FakeArchiveHandler();
	virtual void Dispose();
	virtual bool IsArchive(const wchar_t *path) const; 
	virtual MGDF::IFile *MapArchive(MGDF::IFile *parent,const wchar_t * archiveFile);

private:
	std::vector<std::wstring> _fileExtensions;
	MGDF::ILogger *_logger;
	MGDF::IErrorHandler *_errorHandler;

	/**
	get the extension of a file
	\return the extension (excluding the preceding '.' if possible, otherwise returns "" if no extension could be found
	*/
	std::wstring GetFileExtension(std::wstring file) const;
};