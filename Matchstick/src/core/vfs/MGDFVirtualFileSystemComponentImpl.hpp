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
	virtual bool MapDirectory(const char * physicalDirectory,const char * logicalDirectory,IFileFilter *filter,bool recursive)=0;
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

		virtual bool AddAlias(const char *alias,const char *logicalDirectory);
		virtual void RemoveAlias(const char *alias);
		virtual IFileIterator *FindFiles(const char *logicalDirectory,IFileFilter *filter,bool recursive) const;
		virtual IFile *GetFile(const char *logicalPath) const;
		virtual IFile *GetRoot() const;
		virtual IFileFilterFactory *GetFilterFactory() const;

		virtual bool MapDirectory(const char * physicalDirectory,const char * logicalDirectory,IFileFilter *filter,bool recursive);

		virtual IVirtualFileSystem *RegisterArchiveHandler(IArchiveHandler *);

	private:
		std::vector<IArchiveHandler *> _archiveHandlers;
		std::vector<IFileFilter *> _directoryMapFilters;
		IFile *_root;
		ILogger *_logger;

		Alias *_aliases;
		filters::FileFilterFactory *_filterFactory;

		void MapDirectory(IFile *parent,IFileFilter *filter);
		void Map(const char *currentPhysicalFile, IFile *parentFile,IFileFilter *filter,bool recursive);

		IFile *CreateLogicalDirectory(std::string s);
		IArchiveHandler *GetArchiveHandler(const char *filename);
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


MGDF_CORE_DLL IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl(HINSTANCE instance,HWND window,ILogger *logger);

}}}