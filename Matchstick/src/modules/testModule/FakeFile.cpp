#include "StdAfx.h"

#include <vector>
#include <algorithm>
#include <sstream>
#include "FakeFile.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

FakeFile::FakeFile( const std::wstring &name, const std::wstring &physicalFile, IFile *parent )
{
	_parent = parent;
	_children = nullptr;
	_name = name;
	_physicalPath = physicalFile;
	_data = nullptr;
	_dataLength = 0;
	_isOpen = false;
	_position = 0;
	_logicalPath = L"";
}

FakeFile::FakeFile( const std::wstring &name, FakeFile *parent, void *data, size_t dataLength )  //nullptr data indicates a folder
{
	_parent = parent;
	_children = nullptr;
	_name = name;
	_physicalPath = parent->_physicalPath;
	_data = data;
	_dataLength = dataLength;
	_isOpen = false;
	_position = 0;
	_logicalPath = L"";
}

FakeFile::~FakeFile()
{
	if ( _children != nullptr ) {
		//delete all the children of this node
		for ( auto child : *_children ) {
			delete child.second;
		}
		delete _children;
	}

	delete[]( char * ) _data;
}

MGDF::IFile *FakeFile::GetParent() const
{
	return _parent;
}

size_t FakeFile::GetChildCount() const
{
	if ( _children != nullptr ) {
		return _children->size();
	}
	return 0;
}

time_t FakeFile::GetLastWriteTime() const
{
	return 0;
}

MGDF::IFile *FakeFile::GetChild( const wchar_t * name ) const
{
	if ( !_children || !name ) return nullptr;

	auto it = _children->find( name );
	if ( it != _children->end() ) {
		return it->second;
	}
	return nullptr;
}

bool FakeFile::GetAllChildren( const MGDF::IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const
{
	if ( !_children || !bufferLength ) {
		*bufferLength = 0;
		return 0;
	}

	size_t size = 0;
	for ( auto child : *_children ) {
		if ( !filter || filter->Accept( child.first ) ) {
			if ( size < *bufferLength ) childBuffer[size] = child.second;
			++size;
		}
	}

	bool result = size <= *bufferLength;
	*bufferLength = size;
	return result;
}

void FakeFile::AddChild( FakeFile *file )
{
	_ASSERTE( file );
	if ( !_children ) {
		_children = new std::map<const wchar_t *, FakeFile *,WCharCmp>();
	}
	_children->insert( std::pair<const wchar_t *, FakeFile *> ( file->GetName(), file ) );
}

const wchar_t *FakeFile::GetLogicalPath() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _logicalPath.empty() ) {
		std::vector<const IFile *> path;
		const IFile *node = this;
		while ( node ) {
			path.push_back( node );
			node = node->GetParent();
		}

		std::wostringstream ss;
		for ( auto file : path ) {
			ss << file->GetName();
			if ( file != this ) ss << '/';
		}
		_logicalPath = ss.str();
	}

	return _logicalPath.c_str();
}

bool FakeFile::IsOpen() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _isOpen;
}

MGDF::MGDFError FakeFile::Open( IFileReader **reader )
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _data && !_isOpen ) {
		_isOpen = true;
		_position = 0;
		*reader = this;
		return MGDF::MGDF_OK;
	}
	return MGDF::MGDF_ERR_FILE_IN_USE;
}

void FakeFile::Close()
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _data && _isOpen ) {
		_isOpen = false;
	}
}

UINT32 FakeFile::Read( void* buffer, UINT32 length )
{
	if ( _isOpen ) {
		INT32 oldPosition = _position;
		if ( ( static_cast<UINT32>( oldPosition ) + length ) > _dataLength ) length = static_cast<INT32>( _dataLength ) - oldPosition;
		memcpy( buffer, & ( ( char * ) _data ) [oldPosition], length );
		_position = oldPosition + static_cast<INT32>( length );
		return _position;
	}
	return 0;
}

void FakeFile::SetPosition( INT64 pos )
{
	if ( _isOpen ) {
		_position = static_cast<INT32>( pos );
	}
}

INT64 FakeFile::GetPosition() const
{
	if ( _isOpen ) {
		return _position;
	} else {
		return 0;
	}
}

bool FakeFile::EndOfFile() const
{
	if ( _isOpen ) {
		return _position == _dataLength;
	} else {
		return true;
	}
}

INT64 FakeFile::GetSize() const
{
	return _dataLength;
}

bool FakeFile::IsFolder() const
{
	return _data == nullptr;
}

bool FakeFile::IsArchive() const
{
	return true;
}

const wchar_t *FakeFile::GetArchiveName() const
{
	return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetPhysicalPath() const
{
	return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetName() const
{
	return _name.c_str();
}