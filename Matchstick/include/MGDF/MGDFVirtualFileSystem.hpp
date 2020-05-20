#pragma once

namespace MGDF {

/**
 Provides an interface for reading data from a file
 */
MIDL_INTERFACE("3C07D9A2-D197-4CDC-9DF6-3D244B854FB1")
IFileReader : public IUnknown {
 public:
  /**
  reads the specified number of bytes into the buffer
  \param buffer a buffer to store the read data
  \param length the max amount of data that can be read into the buffer
  \return the amount of bytes actually read into the buffer
  */
  virtual UINT32 STDMETHODCALLTYPE Read(void *buffer, UINT32 length) = 0;

  /**
  sets the read position of the file in bytes
  \param pos the read position of the file in bytes
  */
  virtual void STDMETHODCALLTYPE SetPosition(INT64 pos) = 0;

  /**
  get the current read position of the file in bytes
  \return the read position in the file
  */
  virtual INT64 STDMETHODCALLTYPE GetPosition() const = 0;

  /**
  determines whether the file read position has reached the end of the file
  \return true if the read position is past the end of the file
  */
  virtual bool STDMETHODCALLTYPE EndOfFile() const = 0;

  /**
  get the size of the file in bytes
  \return the filesize in bytes (for compressed archives this value is the
  uncompressed size)
  */
  virtual INT64 STDMETHODCALLTYPE GetSize() const = 0;
};

/**
Provides an interface for navigating a file/directory structure in the virtual
filesystem. Directories have no data but can have subfiles Files have no
subfiles (except for archives) but may have data. Archives that have been added
to the vfs are mapped as files with the same name as the archive filename, but
unlike normal files they also have a tree of subdirectories containing the
uncompressed archive data
*/
MIDL_INTERFACE("7EB85F59-65C6-45AC-B505-F263E1771C7F")
IFile : public IUnknown {
 public:
  /**
  Gets the name of this file
  \return the name of this file
  */
  virtual const wchar_t *STDMETHODCALLTYPE GetName() const = 0;

  /**
  Gets the parent of this file. If this file is the root of the virtual
  filesystem, then this will be nullptr
  \param parent pointer to the parent of this file.
  \return If this file is the root of the virtual filesystem, then return false
  */
  virtual bool STDMETHODCALLTYPE GetParent(IFile * *parent) = 0;

  /**
  Gets a child of this file with the given name (if any)
  \param name the child name of this file
  \return the child file of the current file. If no such file exists, nullptr is
  returned
  */
  virtual bool STDMETHODCALLTYPE GetChild(const wchar_t *name,
                                          IFile **child) = 0;

  /**
   Get all the children of this file (non-recursive)
   \param childBuffer an array to store the results (must be large enough to
   hold GetChildCount entries)
   */
  virtual void STDMETHODCALLTYPE GetAllChildren(IFile * *childBuffer) = 0;

  /**
  Get how many children this file has
  \return how many children this file has
  */
  virtual size_t STDMETHODCALLTYPE GetChildCount() = 0;

  /**
  determines if the file entity is a folder
  \return true if the file is a folder
  */
  virtual bool STDMETHODCALLTYPE IsFolder() const = 0;

  /**
  determines if the file has an open reader
  \return true if the file has an open reader
  */
  virtual bool STDMETHODCALLTYPE IsOpen() const = 0;

  /**
  attempt to open the file for reading. Only one fileReader can be open for a
  file at any time. Because of this, you should ensure that any reader is closed
  after it is no longer needed. \param reader will point to any reader that is
  created
  \return nullptr if the file cannot be opened or is already open
  */
  virtual HRESULT STDMETHODCALLTYPE Open(IFileReader * *reader) = 0;

  /**
  determines if the file is a (or is a member of) an archive file
  \return true if the file is a (or is a member of) an archive file
  */
  virtual bool STDMETHODCALLTYPE IsArchive() const = 0;

  /**
  gets the name of the archive the file belongs to (if any)
  \returns the name of the archive file the entity belongs to, otherwise it
  returns nullptr
  */
  virtual const wchar_t *STDMETHODCALLTYPE GetArchiveName() const = 0;

  /**
  get the pyshical path to the IFile in the filesystem
  \return the pyshical path to the IFile in the filesystem, in the case of an
  archive submember this will be the address to the containing archive
  */
  virtual const wchar_t *STDMETHODCALLTYPE GetPhysicalPath() const = 0;

  /**
  get the path to the file as expressed as a vfs logical file path (i.e. paths
  are relative to the vfs root)
  \return the path to the file as expressed as a vfs logical file path
  */
  virtual const wchar_t *STDMETHODCALLTYPE GetLogicalPath() = 0;

  /**
  find the last write time of the file
  \return a timestamp indicating the last write time
  */
  virtual INT64 STDMETHODCALLTYPE GetLastWriteTime() const = 0;
};

/**
Provides an interface for creating portions of the vfs tree from archive files
*/
MIDL_INTERFACE("09A21A95-9E64-4CCE-8CA3-CD938D94DD4D")
IArchiveHandler : public IUnknown {
 public:
  /**
  get the root node of the mapped vfs subtree
  \param name the name of the archive
  \param physicalPath the physical path on disk containing the archive
  \param parent the VFS parent node of the archive
  \param root pointer to the root node of the mapped vfs subtree
  \return whether the archive could be mapped correctly
  */
  virtual HRESULT STDMETHODCALLTYPE MapArchive(const wchar_t *name,
                                               const wchar_t *physicalPath,
                                               IFile *parent, IFile **root) = 0;

  /**
  whether this file/directory is recognised as an archive type
  \param physicalPath the physical path on disk of a possible achive file
  \return true if the file is a recognized archive file to be handled by this
  handler
  */
  virtual bool STDMETHODCALLTYPE IsArchive(const wchar_t *physicalPath)
      const = 0;
};

/**
Provides an interface for accessing the virtual filesystem, which is a fast read
only interface to access game content files. The root MGDF virtual filesystem is
mounted from the game/content folder.
*/
MIDL_INTERFACE("235AB71E-BAE5-4ACB-BA98-82086378C939")
IVirtualFileSystem : public IUnknown {
 public:
  /**
  Get the file/folder/archive in the specified logical directory. paths are
  delimited using the / character and names are case sensitive. The
  /game/content folder in the MGDF install root folder is mounted as the root of
  the logical filesystem, and no access outside of this folder is permitted.
  \param logicalPath the vfs path to the file
  \param file the file/folder/archive in the specified logical directory. paths
  are delimited using the / character and names are case sensitive \return true
  if the file could be found
  */
  virtual bool STDMETHODCALLTYPE GetFile(const wchar_t *logicalPath,
                                         IFile **file) = 0;

  /**
  Get the root node of the virtual filesystem. In the physical filesystem this
  corresponds to the /game/content folder.
  \param root pointer to the root node of the virtual filesystem
  */
  virtual void STDMETHODCALLTYPE GetRoot(IFile * *root) = 0;
};

}  // namespace MGDF