#include "stdafx.h"
#include <boost/tokenizer.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string.hpp>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFExceptions.hpp"
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
	boost::filesystem::wpath path( parent->GetPhysicalPath(), boost::filesystem::native );
	_ASSERTE( boost::filesystem::is_directory( path ) );

	boost::mutex::scoped_lock( _mutex );

	boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
	for ( boost::filesystem::directory_iterator itr( path ); itr != end_itr; ++itr ) {
		const wchar_t *childName = ( *itr ).path().native().c_str();
		IFile *mappedChild = Map( childName, parent );
		_ASSERTE( mappedChild );
		children.insert( std::pair<const wchar_t *, IFile *> ( mappedChild->GetName(), mappedChild ) );
	}
}


IFile *VirtualFileSystemComponent::Map( const wchar_t *physicalPath, IFile *parent )
{
	_ASSERTE( physicalPath );
	boost::filesystem::wpath path( physicalPath, boost::filesystem::native );
	if ( boost::filesystem::is_directory( path ) ) {
		return new DefaultFolderImpl( path.filename().native(), path.native(), parent, this );
	} else {
		//if its an archive
		IArchiveHandler *archiveHandler = GetArchiveHandler( physicalPath );
		if ( archiveHandler ) {
			IFile *mappedFile = archiveHandler->MapArchive(
			                        path.filename().native().c_str(),
			                        path.native().c_str(),
			                        parent ); //replace it with the mapped archive tree
			if ( mappedFile ) {
				if ( parent == nullptr ) _rootIsArchive = true;
				//store the archive, so we can pass it back to its handler to clean it up later.
				_mappedArchives.insert( std::pair<IArchiveHandler *, IFile *> ( archiveHandler, mappedFile ) );
				return mappedFile;
			} else {
				LOG( "Unable to map archive " << Resources::ToString( physicalPath ), LOG_ERROR );
			}
		}

		//otherwise its just a plain old file
		return new DefaultFileImpl( path.filename().native(), path.native(), parent, _errorHandler );
	}
}

IArchiveHandler *VirtualFileSystemComponent::GetArchiveHandler( const wchar_t *fullFilePath )
{
	_ASSERTE( fullFilePath );
	for ( auto handler : _archiveHandlers ) {
		if ( handler->IsArchive( fullFilePath ) ) {
			return handler;
		}
	}
	return nullptr;
}

IFile *VirtualFileSystemComponent::GetFile( const wchar_t *logicalPath ) const
{
	if ( !logicalPath ) return _root;

	std::vector<std::wstring> pathComponents;
	boost::split( pathComponents, logicalPath, boost::is_any_of( L"/" ) );

	IFile *node = _root;
	for ( auto it = pathComponents.begin(); it != pathComponents.end(); ++it ) {
		node = node->GetChild( it->c_str() );
		if ( !node ) break;
	}

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