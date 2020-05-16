#pragma once

#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
Provides an interface for filtering files from result sets. Clients should
implement this interface and pass instances into the relevant VFS methods in
order to filter results
*/
class IFileFilter {
 public:
  /**
  Whether to include a file in a set of results
  \param childname the name of the file to be filtered
  \return true if the file is to be included in a result set, and false if it
  should be excluded
  */
  virtual bool Accept(const wchar_t *childname) const = 0;
};

/**
 Provides an interface for reading data from a file
 */
class IFileReader {
 public:
  /**
  closes the file reader, this object cannot be used after this is called
  */
  virtual void Close() = 0;

  /**
  reads the specified number of bytes into the buffer
  \param buffer a buffer to store the read data
  \param length the max amount of data that can be read into the buffer
  \return the amount of bytes actually read into the buffer
  */
  virtual UINT32 Read(void *buffer, UINT32 length) = 0;

  /**
  sets the read position of the file in bytes
  \param pos the read position of the file in bytes
  */
  virtual void SetPosition(INT64 pos) = 0;

  /**
  get the current read position of the file in bytes
  \return the read position in the file
  */
  virtual INT64 GetPosition() const = 0;

  /**
  determines whether the file read position has reached the end of the file
  \return true if the read position is past the end of the file
  */
  virtual bool EndOfFile() const = 0;

  /**
  get the size of the file in bytes
  \return the filesize in bytes (for compressed archives this value is the
  uncompressed size)
  */
  virtual INT64 GetSize() const = 0;
};

/**
Provides an interface for navigating a file/directory structure in the virtual
filesystem. Directories have no data but can have subfiles Files have no
subfiles (except for archives) but may have data. Archives that have been added
to the vfs are mapped as files with the same name as the archive filename, but
unlike normal files they also have a tree of subdirectories containing the
uncompressed archive data
*/
class IFile {
 public:
  /**
  Gets the name of this file
  \return the name of this file
  */
  virtual const wchar_t *GetName() const = 0;

  /**
  Gets the parent of this file. If this file is the root of the virtual
  filesystem, then this will be nullptr \return the parent of this file. If this
  file is the root of the virtual filesystem, then this will be nullptr
  */
  virtual IFile *GetParent() const = 0;

  /**
  Gets a child of this file with the given name (if any)
  \param name the child name of this file
  \return the child file of the current file. If no such file exists, nullptr is
  returned
  */
  virtual IFile *GetChild(const wchar_t *name) const = 0;

  /**
   Get all the children of this file (non-recursive) which match the given
   wildcard filter \param filter a user supplied filter to filter the results
   \param childBuffer an array to store the results
   \param bufferLength the length of the childBuffer. Will be set to the length
   of the buffer required when the method returns
   \return true if the supplied buffer is large enough to hold all the results,
   otherwise returns false and sets the size required in bufferLength.
   */
  virtual bool GetAllChildren(const IFileFilter *filter, IFile **childBuffer,
                              size_t *bufferLength) const = 0;

  /**
  Get how many children this file has
  \return how many children this file has
  */
  virtual size_t GetChildCount() const = 0;

  /**
  determines if the file entity is a folder
  \return true if the file is a folder
  */
  virtual bool IsFolder() const = 0;

  /**
  determines if the file has an open reader
  \return true if the file has an open reader
  */
  virtual bool IsOpen() const = 0;

  /**
  attempt to open the file for reading. Only one fileReader can be open for a
  file at any time. Because of this, you should ensure that any reader is closed
  after it is no longer needed. \param reader will point to any reader that is
  created \return nullptr if the file cannot be opened or is already open
  */
  virtual MGDFError Open(IFileReader **reader) = 0;

  /**
  determines if the file is a (or is a member of) an archive file
  \return true if the file is a (or is a member of) an archive file
  */
  virtual bool IsArchive() const = 0;

  /**
  gets the name of the archive the file belongs to (if any)
  \returns the name of the archive file the entity belongs to, otherwise it
  returns nullptr
  */
  virtual const wchar_t *GetArchiveName() const = 0;

  /**
  get the pyshical path to the IFile in the filesystem
  \return the pyshical path to the IFile in the filesystem, in the case of an
  archive submember this will be the address to the containing archive
  */
  virtual const wchar_t *GetPhysicalPath() const = 0;

  /**
  get the path to the file as expressed as a vfs logical file path (i.e. paths
  are relative to the vfs root) \return the path to the file as expressed as a
  vfs logical file path
  */
  virtual const wchar_t *GetLogicalPath() const = 0;

  /**
  find the last write time of the file
  \return a timestamp indicating the last write time
  */
  virtual time_t GetLastWriteTime() const = 0;
};

/**
Provides an interface for creating portions of the vfs tree from archive files
*/
class IArchiveHandler {
 public:
  /**
  get the root node of the mapped vfs subtree
  \param name the name of the archive
  \param physicalPath the physical path on disk containing the archive
  \param parent the VFS parent node of the archive
  \return the root node of the mapped vfs subtree
  */
  virtual IFile *MapArchive(const wchar_t *name, const wchar_t *physicalPath,
                            IFile *parent) = 0;

  /**
  whether this file/directory is recognised as an archive type
  \param physicalPath the physical path on disk of a possible achive file
  \return true if the file is a recognized archive file to be handled by this
  handler
  */
  virtual bool IsArchive(const wchar_t *physicalPath) const = 0;

  /**
  tells the archive handler to dispose of an archive which it previously created
  \param archive an archive previously created by this handler
  */
  virtual void DisposeArchive(IFile *archive) = 0;

  /**
  tells the archive handler to dispose of itself. The object cannot be used
  after this is called
  */
  virtual void Dispose() = 0;
};

/**
Provides an interface for accessing the virtual filesystem, which is a fast read
only interface to access game content files. The root MGDF virtual filesystem is
mounted from the game/content folder.
*/
class IVirtualFileSystem {
 public:
  /**
  Get the file/folder/archive in the specified logical directory. paths are
  delimited using the / character and names are case sensitive. The
  /game/content folder in the MGDF install root folder is mounted as the root of
  the logical filesystem, and no access outside of this folder is permitted.
  \param logicalPath the vfs path to the file
  \return the file/folder/archive in the specified logical directory. paths are
  delimited using the / character and names are case sensitive
  */
  virtual IFile *GetFile(const wchar_t *logicalPath) const = 0;

  /**
  Get the root node of the virtual filesystem. In the physical filesystem this
  corresponds to the /game/content folder. \return the root node of the virtual
  filesystem
  */
  virtual IFile *GetRoot() const = 0;
};

}  // namespace MGDF