#include "StdAfx.h"

#include <vector>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

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
	std::filesystem::path path( GetPhysicalPath() );
	struct _stat64 fileInfo;
	if (_wstati64(path.c_str(), &fileInfo) != 0) {
		LOG( "Unable to get last write time for " << Resources::ToString(GetPhysicalPath()), LOG_ERROR );
		return 0;
	}
	return fileInfo.st_mtime;
}

IFile *FileBaseImpl::GetChild( const wchar_t * name ) const
{
	if ( !name ) return nullptr;

	{
		std::lock_guard<std::mutex> lock( _mutex );
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
		std::lock_guard<std::mutex> lock( _mutex );
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
	std::lock_guard<std::mutex> lock( _mutex );

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
