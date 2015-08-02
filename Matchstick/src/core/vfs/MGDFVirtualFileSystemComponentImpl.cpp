#include "stdafx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFDefaultFileImpl.hpp"
#include "MGDFDefaultFolderImpl.hpp"


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

using namespace std::tr2::sys;

IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl()
{
	return new VirtualFileSystemComponent();
}

VirtualFileSystemComponent::VirtualFileSystemComponent()
	: _root( nullptr )
	, _rootIsArchive( false )
{
}

VirtualFileSystemComponent::~VirtualFileSystemComponent()
{
	if ( !_rootIsArchive ) {
		delete static_cast<FileBaseImpl *>( _root );
	}

	for ( auto &archive : _mappedArchives ) {
		archive.first->DisposeArchive( archive.second );
	}

	for ( auto handler : _archiveHandlers ) {
		handler->Dispose();
	}
}

bool VirtualFileSystemComponent::Mount( const wchar_t *physicalDirectory )
{
	_ASSERTE( physicalDirectory );
	_ASSERTE( !_root );
	_root = Map( physicalDirectory, nullptr );
	return _root != nullptr;
}

//used by folders to lazily enumerate thier children as needed.
void VirtualFileSystemComponent::MapChildren( DefaultFolderImpl *parent, std::map<const wchar_t *, IFile *, WCharCmp> &children )
{
	_ASSERTE( parent );
	path path( parent->GetPhysicalPath() );
	_ASSERTE( is_directory( path ) );

	directory_iterator end_itr; // default construction yields past-the-end
	for ( directory_iterator itr( path ); itr != end_itr; ++itr ) {
		IFile *mappedChild = Map( ( *itr ).path(), parent );
		_ASSERTE( mappedChild );
		children.insert( std::pair<const wchar_t *, IFile *> ( mappedChild->GetName(), mappedChild ) );
	}
}


IFile *VirtualFileSystemComponent::Map( const path &path, IFile *parent )
{
	//wpath path( physicalPath );
	if ( is_directory( path ) ) {
		return new DefaultFolderImpl( path.filename(), path.wstring(), parent, this );
	} else {
		//if its an archive
		IArchiveHandler *archiveHandler = GetArchiveHandler( path.wstring() );
		if ( archiveHandler ) {
			auto filename = path.filename();
			auto fullpath = path.wstring();
			IFile *mappedFile = archiveHandler->MapArchive(
									filename.c_str(),
			                        fullpath.c_str(),
			                        parent ); //replace it with the mapped archive tree
			if ( mappedFile ) {
				if ( parent == nullptr ) _rootIsArchive = true;
				//store the archive, so we can pass it back to its handler to clean it up later.
				_mappedArchives.insert( std::pair<IArchiveHandler *, IFile *> ( archiveHandler, mappedFile ) );
				return mappedFile;
			} else {
				LOG( "Unable to map archive " << Resources::ToString( path.wstring() ), LOG_ERROR );
			}
		}

		//otherwise its just a plain old file
		return new DefaultFileImpl( path.filename(), path.wstring(), parent, _errorHandler );
	}
}

IArchiveHandler *VirtualFileSystemComponent::GetArchiveHandler( const std::wstring &path )
{
	for ( auto handler : _archiveHandlers ) {
		if ( handler->IsArchive( path.c_str() ) ) {
			return handler;
		}
	}
	return nullptr;
}

IFile *VirtualFileSystemComponent::GetFile( const wchar_t *logicalPath ) const
{
	if ( !logicalPath ) return _root;

	IFile *node = _root;

	wchar_t *context = 0;
	size_t destinationLength = wcslen( logicalPath ) + 1;
	wchar_t *copy = new wchar_t[destinationLength];
	wcscpy_s( copy, destinationLength, logicalPath );
	wchar_t *components = wcstok_s( copy, L"/", &context );

	while ( components ) {
		node = node->GetChild( components );
		if ( !node ) break;
		components = wcstok_s( 0, L"/", &context );
	}

	delete[] copy;
	return node;//return the node found (if any)
}

IFile *VirtualFileSystemComponent::GetRoot() const
{
	return _root;
}

void VirtualFileSystemComponent::RegisterArchiveHandler( IArchiveHandler *handler )
{
	_ASSERTE( handler );
	_archiveHandlers.push_back( handler );
}


}
}
}