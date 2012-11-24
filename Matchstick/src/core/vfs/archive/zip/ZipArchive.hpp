#pragma once

#include <vector>
#include <boost/unordered_map.hpp>
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
		INT64 size;
		std::wstring name;
	};

	struct ZipFileData {
		INT64 readPosition;
		char *data;
	};

	virtual ~ZipArchive();
	virtual IFile *MapArchive(IFile *parent,const wchar_t * archiveFile);

	IFile *GetArchiveRoot();
	void DecRefCount();
	void IncRefCount();
	UINT32 GetRefCount();

	bool OpenFile(UINT32 key);
	void CloseFile(UINT32 key);
	ZipFileData *GetFileData(UINT32 key);
	ZipFileInformation *GetFileInformation(UINT32 key);

private:
	UINT32 _refCount;
	std::string _physicalArchiveName;
	unzFile _zip;
	IFile *_archiveRoot;
	ILogger *_logger;
	IErrorHandler *_errorHandler;
	std::vector<ZipFileInformation *> _archiveFiles;//information is loaded for all files in the archive as soon as it is mapped
	boost::unordered_map<UINT32,ZipFileData *> _archiveData;//the data however is loaded lazily and can be removed as necessary to save memory
															//the integer key corresponds to the location in teh archiveFiles vector
	ZipArchive(ILogger *logger,IErrorHandler *errorHandler);
	IFile *CreateParentFile(const std::wstring &path,IFile *rootNode,std::wstring *filename);
};

}}}}