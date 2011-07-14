#pragma once

#include <hash_map>
#include <unzip.h>

#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs { namespace zip {

/** 
handles the mapping and access to zip archives by the virtual file system
*/
class ZipArchive
{
	friend class ZipArchiveHandlerImpl;//only allow the factory to construct instances
public:
	struct ZipFileInformation {
		unz_file_pos filePosition;
		unsigned long size;
		std::wstring name;
	};

	struct ZipFileData {
		unsigned long readPosition;
		char *data;
	};

	virtual ~ZipArchive();
	virtual IFile *MapArchive(IFile *parent,const wchar_t * archiveFile);

	IFile *GetArchiveRoot();
	void DecRefCount();
	void IncRefCount();
	unsigned int GetRefCount();

	bool OpenFile(unsigned int key);
	void CloseFile(unsigned int key);
	ZipFileData *GetFileData(unsigned int key);
	ZipFileInformation *GetFileInformation(unsigned int key);

private:
	unsigned int _refCount;
	std::string _physicalArchiveName;
	unzFile _zip;
	IFile *_archiveRoot;
	ILogger *_logger;
	IErrorHandler *_errorHandler;
	std::vector<ZipFileInformation *> _archiveFiles;//information is loaded for all files in the archive as soon as it is mapped
	stdext::hash_map<unsigned int,ZipFileData *> _archiveData;//the data however is loaded lazily and can be removed as necessary to save memory
															//the integer key corresponds to the location in teh archiveFiles vector
	ZipArchive(ILogger *logger,IErrorHandler *errorHandler);
	IFile *CreateParentFile(std::wstring &path,IFile *rootNode,std::wstring *filename);
};

}}}}