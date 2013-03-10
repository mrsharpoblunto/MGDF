#pragma once

#include <list>
#include <boost/unordered_map.hpp>
#include <unzip.h>

#include "ZipFileRoot.hpp"
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF
{
namespace core
{
namespace vfs
{
namespace zip
{

struct ZipFileHeader {
	unz_file_pos filePosition;
	INT64 size;
	std::wstring name;
};

struct ZipFileData {
	INT64 readPosition;
	char *data;
};

/**
handles the mapping and access to zip archives by the virtual file system
*/
class ZipArchive
{
public:
	ZipArchive( IErrorHandler *errorHandler );
	virtual ~ZipArchive();

	ZipFileRoot *MapArchive( const wchar_t *name, const wchar_t * physicalPath, IFile *parent );

	ZipFileRoot *GetArchiveRoot() const {
		return _root;
	}
	bool GetFileData( ZipFileHeader &header, ZipFileData &data );
private:
	unzFile _zip;
	ZipFileRoot *_root;
	IErrorHandler *_errorHandler;

	IFile *CreateParentFile( std::wstring &path, IFile *root, const wchar_t ** );
};

}
}
}
}