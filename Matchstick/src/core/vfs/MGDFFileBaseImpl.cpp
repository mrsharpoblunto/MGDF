#include "StdAfx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include "MGDFFileBaseImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

FileBaseImpl::FileBaseImpl() 
{
	_parent = NULL;
	_children = NULL;
	_logicalPath = L"";
}

FileBaseImpl::~FileBaseImpl()
{
	if (_children!=NULL) {
		//delete all the children of this node
		boost::unordered_map<std::wstring,IFile *>::iterator iter;
		for (iter=_children->begin();iter!=_children->end();++iter) {
			delete iter->second;
		}
		delete _children;
	}
}

void FileBaseImpl::Dispose()
{
	delete this;
}

IFile *FileBaseImpl::GetParent() const
{
	return _parent;
}

IFile *FileBaseImpl::GetDescendant(const wchar_t * query)
{
	IFile *node=const_cast<IFile *>((const IFile *)this);
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

IFile *FileBaseImpl::GetFirstChild()
{
	if (_children!=NULL && _children->begin() != _children->end()) { 
		return _children->begin()->second;
	}
	return NULL;
}

IFile *FileBaseImpl::GetLastChild()
{
	if (_children!=NULL && _children->begin() != _children->end()) { 
		return _children->end()->second;
	}
	return NULL;
}

IFile *FileBaseImpl::GetChild(const wchar_t * name)
{
	std::wstring n = name;
	std::transform(n.begin(), n.end(), n.begin(), ::towlower);
	if (_children!=NULL && _children->find(n) != _children->end()) {
		return (*_children)[n];
	}
	return NULL;
}

IFile *FileBaseImpl::GetChildInternal(const wchar_t *name)
{
	std::wstring n = name;
	if (_children!=NULL && _children->find(n) != _children->end()) {
		return (*_children)[n];
	}
	return NULL;
}

unsigned int FileBaseImpl::GetChildCount()
{
	if (_children!=NULL) { 
		return _children->size();
	}
	return 0;
}

void FileBaseImpl::AddChild(IFile *file)
{
	//lazily initialise the child map
	if (_children==NULL) {
		_children = new boost::unordered_map<std::wstring,IFile *>();
	}
	//if an identical node already exists in the tree then remove it
	std::wstring fileName = file->GetName();
	(*_children)[fileName] = file; //then insert the new node
}

IFileIterator *FileBaseImpl::GetIterator(void) {
	IFileIterator *result;
	if (_children!=NULL) {
		result = new FileBaseImplIterator(_children->begin(),_children->end());
	}
	else {
		result = new FileBaseImplIterator();
	}
	return result;
}

void FileBaseImpl::SetParent(IFile *file) 
{
	this->_parent = file;
}

const wchar_t *FileBaseImpl::GetLogicalPath()
{
	if (_logicalPath.empty()) {
		IFile *f = (IFile *)this;

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

IFile *FileBaseImplIterator::Current() const
{
	return _mapIter->second;
}

IFile *FileBaseImplIterator::Next()
{
	return (_mapIter++)->second;
}

bool FileBaseImplIterator::HasNext() const
{
	return _isEmpty?false:(_mapIter!=_mapIterEnd);
}

void FileBaseImplIterator::Dispose()
{
	delete this;
}

}}}
