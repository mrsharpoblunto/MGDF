#pragma once

#include <mutex>
#include <string>
#include <map>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF
{
namespace core
{
namespace vfs
{

struct WCharCmp {
	bool operator()( const wchar_t *a, const wchar_t *b ) const {
		return std::wcscmp( a, b ) < 0;
	}
};

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FileBaseImpl : public IFile
{
public:
	FileBaseImpl( IFile *parent );
	virtual ~FileBaseImpl();

	IFile *GetParent() const override final {
		return _parent;
	}
	IFile *GetChild( const wchar_t *name ) const override;

	size_t GetChildCount() const override {
		if ( !_children ) {
			std::lock_guard<std::mutex> lock( _mutex );
			if ( !_children ) {
				return 0;
			}
		}
		return _children->size();
	}

	bool GetAllChildren( const IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const override;
	
	bool IsArchive() const override {
		return false;
	}

	const wchar_t* GetLogicalPath() const override final; 
	time_t GetLastWriteTime() const override;

	// These internal methods are not threadsafe, so ensure 
	// that the mutex for this file is acquired or that only
	// one thread can access the file before calling
	void AddChild( IFile *newNode );
	void SetParent( IFile *file );
protected:
	mutable std::mutex _mutex;
	mutable std::map<const wchar_t *, IFile *, WCharCmp> *_children;
	mutable std::wstring _logicalPath;

	IFile *_parent;
};


}
}
}