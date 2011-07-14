#include "StdAfx.h"

#include <cctype>//std::tolower
#include <algorithm>
#include "FakeFile.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

FakeFile::FakeFile(IFile *parent,std::wstring physicalFile,std::wstring name)
{
	_parent = parent;
	_children = NULL;
	_name = name;
	_physicalPath = physicalFile;
	_data = NULL;
	_dataLength = 0;
	_isOpen = false;
	_position = 0;
	_logicalPath = L"";
}

FakeFile::FakeFile(FakeFile *parent,std::wstring name,void *data,int dataLength)//NULL data indicates a folder
{
	_parent = parent;
	_children = NULL;
	_name = name;
	_physicalPath = parent->_physicalPath;
	_data = data;
	_dataLength = dataLength;
	_isOpen = false;
	_position = 0;
	_logicalPath = L"";
}

FakeFile::~FakeFile()
{
	if (_children!=NULL) {
		//delete all the children of this node
		stdext::hash_map<std::wstring,MGDF::IFile *>::iterator iter;
		for (iter=_children->begin();iter!=_children->end();++iter) {
			delete iter->second;
		}
		delete _children;
	}

	if (_data!=NULL) delete[] (char *)_data;
}

void FakeFile::Dispose()
{
	delete this;
}

MGDF::IFile *FakeFile::GetParent() const
{
	return _parent;
}


MGDF::IFile *FakeFile::GetDescendant(const wchar_t * query)
{
	MGDF::IFile *node=const_cast<MGDF::IFile *>((const MGDF::IFile *)this);
	std::wstring q = query;
	//get the query and convert it to lower case
	std::transform(q.begin(), q.end(), q.begin(), ::towlower);
	size_t dotPos = q.find(VFS_PATH_SEPARATOR);

	//loop until no separators are found or a subnode is NULL
	while (dotPos!=std::wstring::npos && node!=NULL) {
		//get the first node namespace
		std::wstring nodeName = q.substr(0,dotPos);
		q = q.substr(dotPos+1,q.length()-1);

		node = node->GetChild(nodeName.c_str());
		dotPos = q.find(VFS_PATH_SEPARATOR);
	}

	if (node!=NULL) {
		node = node->GetChild(q.c_str());
	}

	return node;//return the node found (if any)
}

MGDF::IFile *FakeFile::GetFirstChild()
{
	if (_children!=NULL && _children->begin() != _children->end()) { 
		return _children->begin()->second;
	}
	return NULL;
}

MGDF::IFile *FakeFile::GetLastChild()
{
	if (_children!=NULL && _children->begin() != _children->end()) { 
		return _children->end()->second;
	}
	return NULL;
}

MGDF::IFile *FakeFile::GetChild(const wchar_t * name)
{
	std::wstring n = name;
	std::transform(n.begin(), n.end(), n.begin(), ::towlower);
	if (_children!=NULL && _children->find(n) != _children->end()) {
		return (*_children)[n];
	}
	return NULL;
}

unsigned int FakeFile::GetChildCount()
{
	if (_children!=NULL) { 
		return _children->size();
	}
	return 0;
}

void FakeFile::AddChild(MGDF::IFile *file)
{
	//lazily initialise the child map
	if (_children==NULL) {
		_children = new stdext::hash_map<std::wstring,MGDF::IFile *>();
	}
	//if an identical node already exists in the tree then remove it
	std::wstring fileName = file->GetName();
	(*_children)[fileName] = file; //then insert the new node
}

MGDF::IFileIterator *FakeFile::GetIterator(void) {
	MGDF::IFileIterator *result;
	if (_children!=NULL) {
		result = new FakeFileIterator(_children->begin(),_children->end());
	}
	else {
		result = new FakeFileIterator();
	}
	return result;
}

const wchar_t *FakeFile::GetLogicalPath()
{
	if (_logicalPath.empty()) {
		MGDF::IFile *f = (MGDF::IFile *)this;

		while (f!=NULL) {
			if (f==this) {
				_logicalPath = f->GetName();
			}
			else {
				std::wstring name = f->GetName();
				_logicalPath = name + VFS_PATH_SEPARATOR + _logicalPath;
			}
			f = f->GetParent();
		}
	}
	return _logicalPath.c_str();
}

bool FakeFile::IsOpen() const
{
	return _isOpen;
}

bool FakeFile::OpenFile()
{
	if (_data && !_isOpen) {
		_isOpen = true;
		_position = 0;
	}
	return false;
}

void FakeFile::CloseFile()
{
	if(_data && _isOpen) 
	{
		_isOpen = false;
	}
}

int FakeFile::Read(void* buffer,int length)
{
	if(_isOpen)
	{
		int oldPosition = _position;
		if ((oldPosition+length) > _dataLength) length = _dataLength - oldPosition;
		memcpy(buffer,&((char *)_data)[oldPosition],length);
		_position = oldPosition + length;
		return _position;
	}
	return 0;
}

void FakeFile::SetPosition(unsigned long pos)
{
	if(_isOpen)
	{
		_position = pos;
	}
}

unsigned long FakeFile::GetPosition() const
{
	if(_isOpen) 
	{
		return _position;
	}
	else 
	{
		return 0;
	}
}

bool FakeFile::EndOfFile() const
{
	if (_isOpen)
	{
		return _position == _dataLength;
	}
	else {
		return true;
	}
}

unsigned long FakeFile::GetSize()
{
	return _dataLength;
}

bool FakeFile::IsFolder() const
{ 
	return _data==NULL;
}

bool FakeFile::IsArchive() const
{
	return true;
}

const wchar_t *FakeFile::GetArchiveName() const
{
	return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetPhysicalPath() const
{ 
	return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetName() const
{
	return _name.c_str();
}

MGDF::IFile *FakeFileIterator::Current() const
{
	return _mapIter->second;
}

MGDF::IFile *FakeFileIterator::Next()
{
	return (_mapIter++)->second;
}

bool FakeFileIterator::HasNext() const
{
	return _isEmpty?false:(_mapIter!=_mapIterEnd);
}

void FakeFileIterator::Dispose()
{
	delete this;
}