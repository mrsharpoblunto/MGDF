#include "StdAfx.h"

#include "../common/MGDFResources.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFDefaultFileImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{
namespace vfs
{

DefaultFileImpl::DefaultFileImpl( const std::wstring &name, const std::wstring &physicalPath, IFile *parent, IErrorHandler *handler )
	: FileBaseImpl( parent )
	, _name( name )
	, _path( physicalPath )
	, _fileStream( nullptr )
	, _errorHandler( handler )
	, _filesize( 0 )
{
	_ASSERTE( handler );
}

DefaultFileImpl::~DefaultFileImpl( void )
{
	Close();
}

MGDFError DefaultFileImpl::Open( IFileReader **reader )
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( !_fileStream ) {
		_fileStream = new std::ifstream( _path.c_str(), std::ios::in | std::ios::binary | std::ios::ate );

		if ( _fileStream && !_fileStream->bad() && _fileStream->is_open() ) {
			_filesize = _fileStream->tellg();
			_fileStream->seekg( 0, std::ios::beg );
			*reader = this;
			return MGDF_OK;
		} else {
			delete _fileStream;
			_fileStream = nullptr;
			LOG( "Unable to open file stream for " << Resources::ToString(_path) << " - " << GetLastError(), LOG_ERROR );
			return MGDF_ERR_INVALID_FILE;
		}
	}
	LOG( "File " << Resources::ToString(_path) << " currently in use", LOG_ERROR );
	return MGDF_ERR_FILE_IN_USE;
}

void DefaultFileImpl::Close()
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _fileStream ) {
		_fileStream->close();
		delete _fileStream;
		_fileStream = nullptr;
	}
}

UINT32 DefaultFileImpl::Read( void* buffer, UINT32 length )
{
	if ( _fileStream && buffer && length ) {
		std::ifstream::pos_type oldPosition = _fileStream->tellg();
		_fileStream->read( ( char* ) buffer, length );
		std::ifstream::pos_type newPosition = _fileStream->tellg();
		return static_cast<UINT32>( newPosition - oldPosition );
	}
	return 0;
}

void DefaultFileImpl::SetPosition( INT64 pos )
{
	if ( _fileStream ) {
		_fileStream->seekg( pos );
	}
}

INT64 DefaultFileImpl::GetPosition() const
{
	if ( _fileStream ) {
		return _fileStream->tellg();
	} else {
		return 0;
	}
}

bool DefaultFileImpl::EndOfFile() const
{
	if ( _fileStream ) {
		return _fileStream->eof();
	} else {
		return true;
	}
}

INT64 DefaultFileImpl::GetSize() const
{
	return _filesize;
}

}
}
}
