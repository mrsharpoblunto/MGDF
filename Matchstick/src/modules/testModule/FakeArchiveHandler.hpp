#pragma once

#include <string>
#include <vector>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

class FakeArchiveHandler: public MGDF::IArchiveHandler
{
public:
	FakeArchiveHandler( MGDF::ILogger *logger, MGDF::IErrorHandler *errorHandler );
	virtual ~FakeArchiveHandler();
	virtual void Dispose();
	virtual void DisposeArchive( MGDF::IFile *file );
	virtual bool IsArchive( const wchar_t *path ) const;
	virtual MGDF::IFile *MapArchive( const wchar_t *name, const wchar_t * archiveFile, MGDF::IFile *parent );

private:
	std::vector<const wchar_t *> _fileExtensions;
	MGDF::ILogger *_logger;
	MGDF::IErrorHandler *_errorHandler;

	/**
	get the extension of a file
	\return the extension (excluding the preceding '.' if possible, otherwise returns "" if no extension could be found
	*/
	const wchar_t *GetFileExtension( const wchar_t *file ) const;
};