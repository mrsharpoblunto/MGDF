#include "StdAfx.h"

#include <sstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include "MGDFFileBaseImpl.hpp"


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

FileBaseImpl::FileBaseImpl( IFile *parent )
	: _parent( parent )
	, _children( nullptr )
{
}

FileBaseImpl::~FileBaseImpl()
{
	delete _children;
}

time_t FileBaseImpl::GetLastWriteTime() const
{
	boost::filesystem::path path( GetPhysicalPath() );
	return boost::filesystem::last_write_time( path );
}

IFile *FileBaseImpl::GetChild( const wchar_t * name ) const
{
	if ( !name ) return nullptr;

	{
		boost::mutex::scoped_lock lock( _mutex );
		if ( !_children ) return nullptr;
	}

	auto it = _children->find( name );
	if ( it != _children->end() ) {
		return it->second;
	}
	return nullptr;
}

bool FileBaseImpl::GetAllChildren( const IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const
{
	if ( !bufferLength ) {
		*bufferLength = 0;
		return false;
	}

	{
		boost::mutex::scoped_lock lock( _mutex );
		if ( !_children ) {
			*bufferLength = 0;
			return false;
		}
	}

	size_t size = 0;
	for ( auto it = _children->begin(); it != _children->end(); ++it ) {
		if ( !filter || filter->Accept( it->first ) ) {
			if ( size < *bufferLength ) childBuffer[size] = it->second;
			++size;
		}
	}

	bool result = size <= *bufferLength;
	*bufferLength = size;
	return result;
}

void FileBaseImpl::AddChild( IFile *file )
{
	_ASSERTE( file );
	if ( !_children ) {
		_children = new std::map<const wchar_t *, IFile *, WCharCmp>();
	}
	_children->insert( std::pair<const wchar_t *, IFile *> ( file->GetName(), file ) );
}

const wchar_t *FileBaseImpl::GetLogicalPath() const
{
	boost::mutex::scoped_lock lock( _mutex );

	if ( _logicalPath.empty() && this->GetParent() ) {
		std::vector<const IFile *> path;
		const IFile *node = this;
		while ( node ) {
			path.push_back( node );
			node = node->GetParent();
		}

		std::wostringstream ss;
		for ( auto it = path.rbegin() + 1; it != path.rend(); ++it ) {
			ss << ( *it )->GetName();
			if ( ( *it ) != this ) ss << '/';
		}
		_logicalPath = ss.str();
	}

	return _logicalPath.c_str();
}

}
}
}
