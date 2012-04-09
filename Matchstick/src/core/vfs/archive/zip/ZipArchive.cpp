#include "stdafx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "../../../common/MGDFExceptions.hpp"
#include "../../../common/MGDFResources.hpp"
#include "ZipFileRoot.hpp"
#include "ZipFileImpl.hpp"
#include "ZipFolderImpl.hpp"
#include "ZipArchiveHandlerImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#pragma warning(disable:4345) //disable irrelevant warning about initializing POD types via new() syntax.

namespace MGDF { namespace core { namespace vfs { namespace zip {

#define FILENAME_BUFFER 512

ZipArchive::ZipArchive(ILogger *logger,IErrorHandler *errorHandler): _refCount(0)
{
	_logger = logger;
	_errorHandler = errorHandler;
	_archiveRoot = NULL;
}

ZipArchive::~ZipArchive(){
	for (std::vector<ZipFileInformation *>::iterator iter = _archiveFiles.begin();iter!=_archiveFiles.end();++iter) {
		delete (*iter);
	}

	for (boost::unordered_map<unsigned int,ZipFileData *>::iterator iter = _archiveData.begin();iter!=_archiveData.end();++iter) {
		delete iter->second;
	}

	if (_zip)
		unzClose(_zip);
}

IFile *ZipArchive::GetArchiveRoot()
{
	return _archiveRoot;
}

void ZipArchive::DecRefCount()
{
	if (_refCount>0) {
	--_refCount;
	}
}

void ZipArchive::IncRefCount()
{
	++_refCount;
}

unsigned int ZipArchive::GetRefCount()
{
	return _refCount;
}

IFile *ZipArchive::MapArchive(IFile *parent,const wchar_t * archiveFile) 
{
	_zip = unzOpen(archiveFile);
	
	if (_zip) {
		_archiveRoot = new ZipFileRoot(archiveFile,_logger,_errorHandler);
		((FileBaseImpl *)_archiveRoot)->SetParent(parent);

		// We need to map file positions to speed up opening later
		for (int ret = unzGoToFirstFile(_zip); ret == UNZ_OK; ret = unzGoToNextFile(_zip)) {
			unz_file_info info;
			char fname[FILENAME_BUFFER];
			std::string name;

			unzGetCurrentFileInfo(_zip, &info, fname, FILENAME_BUFFER, NULL, 0, NULL, 0);

			//get the name and convert it to lower case
			name = fname;
			std::transform(name.begin(), name.end(), name.begin(), std::tolower);

			//if the path is for a folder the last element will be a "" element (because all path element names
			//found using zlib include a trailing "/") this means that the entire folder tree will be created
			//in the case of folders, and that the last element will be excluded for files which is the desired behaviour
			std::wstring filename;
			IFile *parentFile = CreateParentFile(Resources::ToWString(name),_archiveRoot,&filename);

			if (info.uncompressed_size > 0) {
				ZipFileInformation *zfInfo = new ZipFileInformation();
				unzGetFilePos(_zip, &zfInfo->filePosition);
				zfInfo->size = info.uncompressed_size;
				zfInfo->name = filename;//the name is the last part of the path

				_archiveFiles.push_back(zfInfo);
				ZipFileImpl *zipFile = new ZipFileImpl(this,(unsigned int)_archiveFiles.size()-1);
				((FileBaseImpl *)zipFile)->SetParent(parentFile);
				((FileBaseImpl *)parentFile)->AddChild(zipFile);
			}
		}
	}
	else {
		std::string message = "Could not open archive ";
		_logger->Add(THIS_NAME,(message+Resources::ToString(archiveFile)).c_str(),LOG_ERROR);
		return NULL;
	}

	return _archiveRoot;
}

IFile *ZipArchive::CreateParentFile(const std::wstring &path,IFile *rootNode,std::wstring *filename) {

	IFile *currentFile = rootNode;

	size_t startIndex=0;
	wchar_t *p = new wchar_t[path.size()+1];
	wcsncpy_s(p,path.size()+1,path.c_str(),path.size());

	for (size_t i = 0;i<path.size();++i) {
		if (p[i]==L'/')
		{
			p[i] = L'\0';
			IFile *child = ((FileBaseImpl *)currentFile)->GetChildInternal(&(p[startIndex]));
			if (child == NULL) {
				std::wstring childName = &p[startIndex];
				child = new ZipFolderImpl(this,childName);
				((FileBaseImpl *)child)->SetParent(currentFile);
				((FileBaseImpl *)currentFile)->AddChild(child);
			}
			currentFile = child;
			startIndex = i+1;
		}
	}

	if (startIndex<path.size()) *filename = &p[startIndex];
	delete[] p;
	return currentFile;
}

bool ZipArchive::OpenFile(unsigned int key)
{
	//if the entry is already in the hashmap then the file is already open
	//if its not in the hashmap then open it
	if (_archiveData.find(key)==_archiveData.end()) {
		unzGoToFilePos(_zip,&_archiveFiles[key]->filePosition);

		ZipFileData *zfData = new ZipFileData();
		zfData->readPosition = 0;
		zfData->data = (char *)malloc(_archiveFiles[key]->size);

		// If anything fails, we abort and return false
		try {
			if (unzOpenCurrentFile(_zip) != UNZ_OK)
				throw MGDFException("Unable to open zip archive");
			if (unzReadCurrentFile(_zip, zfData->data, _archiveFiles[key]->size) < 0) 
				throw MGDFException("Unable to read zip archive");
			if (unzCloseCurrentFile(_zip) == UNZ_CRCERROR)
				throw MGDFException("Unable to close zip archive");

			//if everything has gone okay then add the file to the data cache
			_archiveData[key] = zfData;
			return true;
		}
		catch (MGDFException e) {
			std::string message = e.what();
			_logger->Add(THIS_NAME,(message+" "+Resources::ToString(_archiveFiles[key]->name)).c_str(),LOG_ERROR);
			_errorHandler->SetLastError(THIS_NAME,MGDF_ERR_INVALID_ARCHIVE_FILE,e.what());
			free(zfData->data);
		}
	}
	return false;
}

void ZipArchive::CloseFile(unsigned int key)
{
	boost::unordered_map<unsigned int,ZipFileData *>::iterator iter = _archiveData.find(key);
	//if the entry is in the hashmap then the file is already open, so it needs to be closed and removed from the data cache
	if (iter!=_archiveData.end()) {
		free(iter->second->data);
		delete iter->second;
		_archiveData.erase(iter);
	}
}

ZipArchive::ZipFileData *ZipArchive::GetFileData(unsigned int key)
{
	return _archiveData[key];
}

ZipArchive::ZipFileInformation *ZipArchive::GetFileInformation(unsigned int key)
{
	return _archiveFiles[key];
}

}}}}