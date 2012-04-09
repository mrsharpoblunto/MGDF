#include "stdafx.h"
#include <boost/tokenizer.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string.hpp>

#include <MGDF/MGDF.hpp>

#include "../common/MGDFExceptions.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFAlias.hpp"
#include "MGDFDefaultFileImpl.hpp"
#include "MGDFDefaultFolderImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

#define ROOT_NODE_NAME L"." 

IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl(HINSTANCE instance,HWND window,ILogger *logger)
{
	try 
	{
		return new VirtualFileSystemComponent(instance,window,logger);
	}
	catch (...)
	{
		return NULL;
	}
}

VirtualFileSystemComponent::VirtualFileSystemComponent(HINSTANCE instance,HWND window,ILogger *logger)
{
	_logger = logger;
	_root = new DefaultFolderImpl(ROOT_NODE_NAME);
	_aliases = new Alias();
	_filterFactory = new filters::FileFilterFactory();
}

void VirtualFileSystemComponent::Dispose()
{
	delete this;
}

VirtualFileSystemComponent::~VirtualFileSystemComponent()
{
	for (std::vector<IArchiveHandler *>::iterator iter = _archiveHandlers.begin();iter!=_archiveHandlers.end();++iter) {
		delete (*iter);
	}

	for (std::vector<IFileFilter *>::iterator iter = _directoryMapFilters.begin();iter!=_directoryMapFilters.end();++iter) {
		delete (*iter);
	}

	delete _root;
	delete _aliases;
	delete _filterFactory;
}


bool VirtualFileSystemComponent::MapDirectory(const wchar_t *physicalDirectory,const wchar_t *logicalDirectory,IFileFilter *filter,bool recursive)
{
	IFile *logicalDir = CreateLogicalDirectory(logicalDirectory);
	if (logicalDir==NULL)
	{
		_logger->Add(THIS_NAME,"Unable to create logical directory, a component of the path is a file",LOG_ERROR);
		SAFE_DELETE(filter);
		return false;
	}

	boost::filesystem::wpath physicalDirectoryPath(physicalDirectory,boost::filesystem::native);

	//if the physical object to be mapped is a directory then map the contents of the directory into the logical directory
	//though the actual phsycial directory will not
	if (boost::filesystem::is_directory(physicalDirectoryPath)) {
		boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		for ( boost::filesystem::directory_iterator itr( physicalDirectoryPath ); itr != end_itr; ++itr ) {
			Map((*itr).path().native().c_str() ,logicalDir ,filter,recursive);
		}		
	}
	//else the phsycial object is a file and will be mapped into the logical directory directly
	else {
		Map(physicalDirectoryPath.native().c_str(),logicalDir,filter,recursive);
	}

	//store the filter so that we can use it for later lazy mappings.
	_directoryMapFilters.push_back(filter);

	return true;
}

//used by folders to lazily enumerate thier children as needed.
void VirtualFileSystemComponent::MapDirectory(IFile *parent,IFileFilter *filter)
{
	boost::filesystem::wpath physicalDirectoryPath(parent->GetPhysicalPath(),boost::filesystem::native);

	//if the physical object to be mapped is a directory then map the contents of the directory into the logical directory
	//though the actual phsycial directory will not
	if (boost::filesystem::is_directory(physicalDirectoryPath)) {
		boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		for ( boost::filesystem::directory_iterator itr( physicalDirectoryPath ); itr != end_itr; ++itr ) {
			Map((*itr).path().native().c_str() ,parent ,filter,true);
		}		
	}
	//else the phsycial object is a file and will be mapped into the logical directory directly
	else {
		Map(physicalDirectoryPath.native().c_str(),parent,filter,true);
	}
}


void VirtualFileSystemComponent::Map(const wchar_t *currentPhysicalFile, IFile *parentFile,IFileFilter *filter,bool recursive)
{
	//if the file passes the filter
	if (filter == NULL || filter->FilterFile(currentPhysicalFile)) {
		IFile *currentFile=NULL;

		//if its an archive
		IArchiveHandler *archiveHandler = GetArchiveHandler(currentPhysicalFile);
		if (archiveHandler!=NULL) {
			currentFile = archiveHandler->MapArchive(parentFile,currentPhysicalFile);//replace it with the mapped archive tree
			if (currentFile==NULL)
			{
				//if we can't map it as an archive, thenwe will revert to mapping it as a physical file or folder.
				std::string message = "Unable to map archive ";
				_logger->Add(THIS_NAME,(message+Resources::ToString(std::wstring(currentPhysicalFile))).c_str(),LOG_ERROR);
			}
		}

		if (currentFile==NULL)
		{
			boost::filesystem::wpath physicalDirectoryPath(currentPhysicalFile,boost::filesystem::native);
			//if its a folder then map it into the vfs tree (also note if we want this folder to lazily recusivly map its children)
			if (boost::filesystem::is_directory(physicalDirectoryPath)) {
				if (recursive)
				{
					currentFile = new DefaultFolderImpl(physicalDirectoryPath.filename().native(),physicalDirectoryPath.native(), this,filter);
					((FileBaseImpl *)currentFile)->SetParent(parentFile);
				}
			}
			//if its a normal file
			else {
				currentFile = new DefaultFileImpl(currentPhysicalFile,_logger,_errorHandler);
				((FileBaseImpl *)currentFile)->SetParent(parentFile);
			}
		}

		if (currentFile!=NULL) {
			((FileBaseImpl *)parentFile)->AddChild(currentFile);//add the current file to the vfs tree
		}
	}
}


IArchiveHandler *VirtualFileSystemComponent::GetArchiveHandler(const wchar_t *fullFilePath)
{
	for (std::vector<IArchiveHandler *>::iterator iter=_archiveHandlers.begin();iter!=_archiveHandlers.end();++iter) {
		if ((*iter)->IsArchive(fullFilePath)) {
				return (*iter);
		}
	}
	return NULL;
}


IFile *VirtualFileSystemComponent::CreateLogicalDirectory(const std::wstring &logicalDirectory) {
	std::wstring ldir = _aliases->ResolveAliases(logicalDirectory);//remove any aliases present
	IFile *currentDirectory = _root;//start the search at the root node

	//tokenize the path e.g a/file/path becomes a list with (a,file,path)
	std::vector<std::wstring> splitPath;
	boost::split(splitPath,ldir,boost::is_any_of(VFS_PATH_SEPARATOR));

	//remove the root node from the query string if present
	if (splitPath.size()>0 && splitPath[0]==ROOT_NODE_NAME) {
		splitPath.erase(splitPath.begin());
	}

	for (std::vector<std::wstring>::iterator it = splitPath.begin(); it != splitPath.end(); ++it)
	{
		if (*it != L"") {
			if (!currentDirectory->IsFolder()) {
				return NULL;
			}

			//if the child does not exist and the current file is a directory then create a new subdirectory
			if (currentDirectory->GetChild((*it).c_str()) == NULL) {
				IFile *newDir = new DefaultFolderImpl(*it);
				((FileBaseImpl *)currentDirectory)->AddChild(newDir);
				((FileBaseImpl *)newDir)->SetParent(currentDirectory);
			}

			currentDirectory = currentDirectory->GetChild((*it).c_str());
		}
	}

	return currentDirectory;
}

bool VirtualFileSystemComponent::AddAlias(const wchar_t *alias,const wchar_t *logicalDirectory)
{
	try 
	{
		_aliases->AddAlias(alias,logicalDirectory);
		return true;
	}
	catch (EAliasSyntaxException ex)
	{
		return false;
	}
}

void VirtualFileSystemComponent::RemoveAlias(const wchar_t *alias)
{
	_aliases->RemoveAlias(alias);
}

IFileIterator *VirtualFileSystemComponent::FindFiles(const wchar_t *logicalDirectory,IFileFilter *filter,bool recursive) const
{
	IFile *logicalDir = this->GetFile(logicalDirectory);
	std::vector<IFile *> *files = new std::vector<IFile *>();

	if (logicalDir!=NULL) {
		std::auto_ptr<IFileIterator> iter(logicalDir->GetIterator());

		while (iter->HasNext()) {
			FindFilesRecursive(iter->Next(),filter,recursive,files);
		}
	}

	return new FoundFilesIterator(files);
}

void VirtualFileSystemComponent::FindFilesRecursive(IFile *currentDirectory,IFileFilter *filter,bool recursive,std::vector<IFile *> *files) const
{
	if (filter==NULL || filter->FilterFile(currentDirectory->GetName())) {
		files->push_back(currentDirectory);
	}

	//if recursive is not set then don't traverse down into subdirectories
	if (recursive) {
		std::auto_ptr<IFileIterator> iter(currentDirectory->GetIterator());
		while (iter->HasNext()) {
			FindFilesRecursive(iter->Next(),filter,recursive,files);
		}
	}
}

IFile *VirtualFileSystemComponent::GetFile(const wchar_t *logicalPath) const
{
	std::wstring lPath = logicalPath;

	//if the root is requested
	if (lPath==L".") {
		return _root;
	}

	lPath = _aliases->ResolveAliases(lPath); //remove any aliases present
	std::wstring rootNodeRef = ROOT_NODE_NAME;
	rootNodeRef+=VFS_PATH_SEPARATOR;
	boost::algorithm::trim_left_if(lPath,boost::algorithm::is_any_of(rootNodeRef)); //trim off any reference to the root node if present
	if (_root!=NULL) {
		return _root->GetDescendant(lPath.c_str());
	}
	return NULL;
}

IFile *VirtualFileSystemComponent::GetRoot() const
{
	return _root;
}

IVirtualFileSystem *VirtualFileSystemComponent::RegisterArchiveHandler(IArchiveHandler *handler)
{
	_archiveHandlers.push_back(handler);
	return this;//allows for easy chaining of registrations
}

IFileFilterFactory *VirtualFileSystemComponent::GetFilterFactory() const
{
	return _filterFactory;
}

FoundFilesIterator::~FoundFilesIterator()
{
	delete _files;
}

IFile *FoundFilesIterator::Current() const
{
	return *_iter;
}

IFile *FoundFilesIterator::Next()
{
	return *(_iter++);
}

bool FoundFilesIterator::HasNext() const
{
	return _iter!=_files->end();
}

void FoundFilesIterator::Dispose()
{
	delete this;
}


}}}