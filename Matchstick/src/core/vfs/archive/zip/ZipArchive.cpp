#include "stdafx.h"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "../../../common/MGDFExceptions.hpp"
#include "../../../common/MGDFResources.hpp"
#include "../../../common/MGDFLoggerImpl.hpp"
#include "ZipFileRoot.hpp"
#include "ZipFileImpl.hpp"
#include "ZipFolderImpl.hpp"
#include "ZipArchiveHandlerImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#pragma warning(disable:4345) //disable irrelevant warning about initializing POD types via new() syntax.

namespace MGDF
{
namespace core
{
namespace vfs
{
namespace zip
{

#define FILENAME_BUFFER 512

ZipArchive::ZipArchive( IErrorHandler *errorHandler )
	: _errorHandler( errorHandler )
	, _root( nullptr )
{
	_ASSERTE( errorHandler );
}

ZipArchive::~ZipArchive()
{
	if ( _zip )
		unzClose( _zip );
}

ZipFileRoot *ZipArchive::MapArchive( const wchar_t *name, const wchar_t * physicalPath, IFile *parent )
{
	_ASSERTE( name );
	_ASSERTE( physicalPath );

	_zip = unzOpen( physicalPath );

	if ( _zip ) {
		_root = new ZipFileRoot( name, physicalPath, parent, _errorHandler );

		// We need to map file positions to speed up opening later
		for ( INT32 ret = unzGoToFirstFile( _zip ); ret == UNZ_OK; ret = unzGoToNextFile( _zip ) ) {
			unz_file_info info;
			char name[FILENAME_BUFFER];

			unzGetCurrentFileInfo( _zip, &info, name, FILENAME_BUFFER, nullptr, 0, nullptr, 0 );

			//if the path is for a folder the last element will be a "" element (because all path element names
			//found using zlib include a trailing "/") this means that the entire folder tree will be created
			//in the case of folders, and that the last element will be excluded for files which is the desired behaviour
			const wchar_t *filename = nullptr;
			std::wstring path = Resources::ToWString( name );
			IFile *parentFile = CreateParentFile( path, _root, &filename );

			if ( info.uncompressed_size > 0 ) {
				_ASSERTE( filename );
				ZipFileHeader header;
				unzGetFilePos( _zip, &header.filePosition );
				header.size = info.uncompressed_size;
				header.name = filename;//the name is the last part of the path

				ZipFileImpl *zipFile = new ZipFileImpl( parentFile, this, std::move( header ) );
				static_cast<FileBaseImpl *>( parentFile )->AddChild( zipFile );
			}
		}
	} else {
		LOG( "Could not open archive " << Resources::ToString( physicalPath ), LOG_ERROR );
		return nullptr;
	}

	return _root;
}

IFile *ZipArchive::CreateParentFile( std::wstring &path, IFile *root, const wchar_t **filename )
{
	_ASSERTE( root );
	_ASSERTE( path.size() );

	size_t len = path.rfind( '/' );
	if ( len == std::wstring::npos ) {
		*filename = path.data();
		len = 0;
	} else {
		path[len] = '\0';
		*filename = &path.data() [len + 1];
	}

	size_t start = 0;
	size_t end = 0;
	IFile *parent = root;

	while ( end < len ) {
		while ( end < len && path[end] != '/' ) {
			++end;
		}
		if ( end != start ) {
			path[end] = '\0';
			IFile *child = parent->GetChild( &path[start] );
			if ( !child ) {
				child = new ZipFolderImpl( &path[start], parent, this );
				static_cast<FileBaseImpl *>( parent )->AddChild( child );
			}
			parent = child;
		}
		++end;
		start = end;
	}

	return parent;
}

MGDFError ZipArchive::GetFileData( ZipFileHeader &header, ZipFileData &data )
{
	//if the entry is already in the map then the file is already open
	//if its not in the hashmap then open it
	unzGoToFilePos( _zip, &header.filePosition );


	if ( header.size > UINT32_MAX ) {
		std::string message = "Archive files cannot be over 4GB in size";
		LOG( "Archive files cannot be over 4GB in size " << Resources::ToString( header.name ), LOG_ERROR );
		return MGDF_ERR_ARCHIVE_FILE_TOO_LARGE;
	}

	data.readPosition = 0;
	data.data = ( char * ) malloc( static_cast<UINT32>( header.size ) );

	// If anything fails, we abort and return false
	try 
	{
		if ( unzOpenCurrentFile( _zip ) != UNZ_OK )
			throw MGDFException( "Unable to open zip archive" );
		if ( unzReadCurrentFile( _zip, data.data, static_cast<UINT32>( header.size ) ) < 0 )
			throw MGDFException( "Unable to read zip archive" );
		if ( unzCloseCurrentFile( _zip ) == UNZ_CRCERROR )
			throw MGDFException( "Unable to close zip archive" );
		return MGDF_OK;
	} catch ( MGDFException e ) {
		std::string message = e.what();
		LOG( e.what() << ' ' << Resources::ToString( header.name ), LOG_ERROR );
		free( data.data );
		data.data = nullptr;
		return MGDF_ERR_INVALID_ARCHIVE_FILE;
	}
}

}
}
}
}