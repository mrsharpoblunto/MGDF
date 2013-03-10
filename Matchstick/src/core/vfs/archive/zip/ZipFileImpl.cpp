#include "StdAfx.h"

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
	CloseFile();
}

bool ZipFileImpl::OpenFile()
{
	if ( !_isOpen ) {
		_isOpen = _handler->GetFileData( _header, _data );
		return _isOpen;
	}
	return false;
}

void ZipFileImpl::CloseFile()
{
	if ( _isOpen ) {
		free( _data.data );
		_isOpen = false;
	}
}

UINT32 ZipFileImpl::Read( void* buffer, UINT32 length )
{
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