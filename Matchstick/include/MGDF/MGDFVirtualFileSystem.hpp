#pragma once

#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFDisposable.hpp>

namespace MGDF {

#define VFS_ALIAS_SEPARATOR L"%"
#define VFS_PATH_SEPARATOR L"/"

/**
 provides a means to filter searches and mappings into the virtual file system
 \author gcconner
 */
class IFileFilter: public IDisposable
{
public:
	/**
	chain another filter after the current one
	\return the chained filter, so another filter can be chanined onto it in turn
	i.e filter1.chainFilter(filter2).chainFilter(filter3).chainFilter(filter4);
	*/
	virtual IFileFilter * ChainFilter(IFileFilter *filter)=0;

	/**
	\return true if the file is to be included in the filtered list of results otherwise returns false
	*/
	virtual bool  FilterFile(const wchar_t *file)=0;
};

/**
this class provides the means to create a number of common filters. If these are insufficient then custom filters can also be create
by implementing the IFileFilter interface
\author gcconner
*/
class IFileFilterFactory
{
public:
	/**
	creates a filter which filters out all instances of files with a matching name
	the name can include regexes
	*/
	virtual IFileFilter * CreateNameExclusionFilter(const wchar_t *name) const=0;

	/**
	creates a filter which filters in all instances of files without a matching name
	the name can include regexes
	*/
	virtual IFileFilter * CreateNameInclusionFilter(const wchar_t *name) const=0;

	/**
	creates a filter which filters in all instance of files with a matching extension
	\param extension the extension to filter (excluding the preceding '.')
	*/
	virtual IFileFilter * CreateFileExtensionExclusionFilter(const wchar_t *extension) const=0;

	/**
	creates a filter which filters out all instance of files with a matching extension
	\param extension the extension to filter (excluding the preceding '.')
	*/
	virtual IFileFilter * CreateFileExtensionInclusionFilter(const wchar_t *extension) const=0;
};

class IFile;

class IFileIterator: public IDisposable { 
	public: 
	virtual IFile * Current() const=0; 
	virtual IFile * Next()=0;
	virtual bool  HasNext() const=0; 
};

/**
represents a file/directory structure in the virtual file system. Directories have no data but can have subfiles
Files have no subfiles (except for archives) but may have data. Archives that have been added to the vfs are mapped 
as files with the same name as the archive filename, but unlike normal files they also have a tree of subdirectories
containing the uncompressed archive data
\author gcconner
*/
class IFile: public IDisposable
{
public:
	virtual const wchar_t * GetName() const=0;
	virtual IFile * GetParent() const=0;
	virtual IFileIterator * GetIterator() =0;
	virtual IFile * GetDescendant(const wchar_t *path) =0;
	virtual IFile * GetFirstChild() =0;
	virtual IFile * GetLastChild() =0;
	virtual IFile * GetChild(const wchar_t *name) =0;
	virtual size_t  GetChildCount() =0;

	/**
	determines if the IFile entity is a folder
	\return true if the IFile is a folder
	*/
	virtual bool  IsFolder() const =0;

	/**
	determines if the IFile is open
	\return true if the IFile is open
	*/
	virtual bool  IsOpen() const=0;

	/**
	attempt to open the file for reading 
	\return false if the file cannot be opened
	*/
	virtual bool OpenFile()=0;

	/**
	closes the file if its open, otherwise does nothing
	*/
	virtual void CloseFile()=0;

	/**
	reads the specified number of bytes into the buffer and returns the amount of bytes actually read into the buffer
	if the file is not open then the method adds nothing to the buffer and returns 0
	if the file has no data or the 
	*/
	virtual UINT32 Read(void* buffer,UINT32 length)=0;

	/**
	sets the read position of the file in bytes (if the file is open)
	*/
	virtual void SetPosition(INT64 pos)=0;

	/**
	get the current read position of the file in bytes
	\return the read position in the file (or 0 if the file is closed)
	*/
	virtual INT64 GetPosition() const=0;

	/**
	determines whether the file read position has reached the end of the file
	\returns true if the read position is past the end of the file (or if the file is closed)
	*/
	virtual bool  EndOfFile() const=0;

	/**
	get the size of the file in bytes
	\return the filesize in bytes (for compressed archives this value is the uncompressed size)
	*/
	virtual INT64 GetSize()=0;

	/**
	determines if the IFile is a (or is a member of) an archive file
	\return true if the entity is a (or is a member of) an archive file
	*/
	virtual bool  IsArchive() const=0;

	/**
	gets the name of the archive the file belongs to (if any)
	\returns the name of the archive file the entity belongs to, otherwise it returns ""
	*/
	virtual const wchar_t * GetArchiveName() const=0;

	/**
	get the pyshical path to the IFile in the filesystem
	\return the pyshical path to the IFile in the filesystem, in the case of an archive submember this will be the address to the containing archive
	*/
	virtual const wchar_t * GetPhysicalPath() const=0;

	/**
	get the path to the IFile as expressed as a vfs logical file path
	\return the path to the IFile as expressed as a vfs logical file path
	*/
	virtual const wchar_t * GetLogicalPath()=0;

	/**
	find the last write time of the file
	\return a timestamp indicating the last write time
	*/
	virtual time_t GetLastWriteTime() const=0;
};

/**
this interface encapsulates the functionality required for creating portions of the vfs tree from archive files
*/
class IArchiveHandler: public IDisposable
{
public:
	/**
	get the root node of the mapped vfs subtree
	\return the root node of the mapped vfs subtree
	*/
	virtual IFile * MapArchive(IFile *parent,const wchar_t *archive)=0;

	/**
	whether this file/directory is recognised as an archive type
	*/
	virtual bool  IsArchive(const wchar_t *path) const=0;
};

/**
this interface provides facilities for building and accessing the virtual filesystem
\author gcconner
*/
class IVirtualFileSystem
{
	public:
		/**
		sets a string alias to a logical directory
		*/
		virtual bool  AddAlias(const wchar_t *alias,const wchar_t *logicalDirectory)=0;

		/**
		removes a string alias from a logical directory
		*/
		virtual void RemoveAlias(const wchar_t *alias)=0;

		/**
		return a list of File objects satisfying the filters provided in the specified logical directory
		*/
		virtual IFileIterator * FindFiles(const wchar_t *logicalDirectory,IFileFilter *filter,bool recursive) const=0;

		/**
		return the file/folder/archive in the denoted logical directory
		*/
		virtual IFile * GetFile(const wchar_t *logicalPath) const=0;

		virtual IFile * GetRoot() const=0;

		virtual const IFileFilterFactory * GetFilterFactory() const=0;
};

}