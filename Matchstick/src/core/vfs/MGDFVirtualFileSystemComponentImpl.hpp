#pragma once

#include <vector>
#include <hash_map>
#include <boost/filesystem/operations.hpp>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include "../common/MGDFListImpl.hpp"
#include "../common/MGDFSystemComponent.hpp"
#include "filters/MGDFFileFilterFactory.hpp"

namespace MGDF { namespace core { namespace vfs {

class IVirtualFileSystemComponent: public ISystemComponent,public IVirtualFileSystem, public IDisposable {
public:
	virtual bool MapDirectory(const wchar_t * physicalDirectory,const wchar_t * logicalDirectory,IFileFilter *filter,bool recursive)=0;
	virtual IVirtualFileSystem *RegisterArchiveHandler(IArchiveHandler *)=0;
};

class Alias;

class DefaultFolderImpl;

class VirtualFileSystemComponent: public DisposeImpl<IVirtualFileSystemComponent>
{
	friend class DefaultFolderImpl;
	public:
		VirtualFileSystemComponent(HINSTANCE instance,HWND window,ILogger *logger);
		virtual ~VirtualFileSystemComponent();
		virtual void Dispose();

		virtual bool AddAlias(const wchar_t *alias,const wchar_t *logicalDirectory);
		virtual void RemoveAlias(const wchar_t *alias);
		virtual IFileIterator *FindFiles(const wchar_t *logicalDirectory,IFileFilter *filter,bool recursive) const;
		virtual IFile *GetFile(const wchar_t *logicalPath) const;
		virtual IFile *GetRoot() const;
		virtual IFileFilterFactory *GetFilterFactory() const;

		virtual bool MapDirectory(const wchar_t * physicalDirectory,const wchar_t * logicalDirectory,IFileFilter *filter,bool recursive);

		virtual IVirtualFileSystem *RegisterArchiveHandler(IArchiveHandler *);

	private:
		std::vector<IArchiveHandler *> _archiveHandlers;
		std::vector<IFileFilter *> _directoryMapFilters;
		IFile *_root;
		ILogger *_logger;

		Alias *_aliases;
		filters::FileFilterFactory *_filterFactory;

		void MapDirectory(IFile *parent,IFileFilter *filter);
		void Map(const wchar_t *currentPhysicalFile, IFile *parentFile,IFileFilter *filter,bool recursive);

		IFile *CreateLogicalDirectory(const std::wstring &s);
		IArchiveHandler *GetArchiveHandler(const wchar_t *filename);
		void FindFilesRecursive(IFile *currentDirectory,IFileFilter *filter,bool recursive,std::vector<IFile *> *files) const; 
};

class FoundFilesIterator: public DisposeImpl<IFileIterator>
{ 
	friend class VirtualFileSystemComponent;
public:
	virtual ~FoundFilesIterator();
	virtual IFile *Current() const;
	virtual IFile *Next();
	virtual bool HasNext() const;
	virtual void Dispose();
private:
	FoundFilesIterator(std::vector<IFile *> *files): _files(files), _iter(files->begin()) {};

	std::vector<IFile *> *_files;
	std::vector<IFile *>::iterator _iter;
};


IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl(HINSTANCE instance,HWND window,ILogger *logger);

}}}