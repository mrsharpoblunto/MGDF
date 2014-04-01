#include "StdAfx.h"
#include <algorithm>

#include "ZipFileImpl.hpp"

// std min&max are used instead of the macros
#ifdef min
#undef min
#undef max
#endif


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
namespace zip
{

ZipFileImpl::~ZipFileImpl()
{
	Close();
}

MGDFError ZipFileImpl::Open( IFileReader **reader )
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( !_isOpen ) {
		MGDFError result = _handler->GetFileData( _header, _data );
		if ( result == MGDF_OK ) {
			_isOpen = true;
			*reader = this;
		}
		return result;
	}
	return MGDF_ERR_FILE_IN_USE;
}

void ZipFileImpl::Close()
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _isOpen ) {
		free( _data.data );
		_isOpen = false;
	}
}

UINT32 ZipFileImpl::Read( void* buffer, UINT32 length )
{
	if ( !buffer ) return 0;

	UINT32 maxRead = 0;
	if ( _isOpen ) {
		maxRead = std::min<UINT32> ( length, static_cast<UINT32>( _header.size - _data.readPosition ) );
		memcpy( buffer, _data.data + _data.readPosition, maxRead );
		_data.readPosition += maxRead;
	}

	return maxRead;
}

void ZipFileImpl::SetPosition( INT64 pos )
{
	if ( _isOpen ) {
		_data.readPosition = pos;
	}
}

INT64 ZipFileImpl::GetPosition() const
{
	if ( _isOpen ) {
		return _data.readPosition;
	}
	return -1;
}

bool ZipFileImpl::EndOfFile() const
{
	if ( _isOpen ) {
		return _data.readPosition >= _header.size;
	}
	return true;
}

}
}
}
}