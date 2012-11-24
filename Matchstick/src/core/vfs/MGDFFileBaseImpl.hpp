#pragma once

#include <string>
#include <boost/unordered_map.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs {

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FileBaseImpl : public DisposeImpl<IFile>
{
public:
	FileBaseImpl();
	virtual ~FileBaseImpl(void);

	virtual void Dispose();
	virtual IFile *GetParent() const;
	virtual IFileIterator *GetIterator();
	virtual IFile *GetDescendant(const wchar_t * query);
	virtual IFile *GetFirstChild();
	virtual IFile *GetLastChild();
	virtual IFile *GetChild(const wchar_t *name);
	virtual size_t GetChildCount();
	virtual const wchar_t* GetLogicalPath();
	virtual time_t GetLastWriteTime() const;

	IFile *GetChildInternal(const wchar_t *name);
	void AddChild(IFile *newNode);
	void SetParent(IFile *file); 
protected:
	boost::unordered_map<std::wstring,IFile *> *_children;
	IFile *_parent;
	std::wstring _logicalPath;
};

class FileBaseImplIterator: public DisposeImpl<IFileIterator>
{ 
	friend class FileBaseImpl;
public:
	virtual ~FileBaseImplIterator(){};
	virtual IFile *Current() const;
	virtual IFile *Next();
	virtual bool HasNext() const;
	virtual void Dispose();
private:
	FileBaseImplIterator() : _isEmpty(true) {};
	FileBaseImplIterator(boost::unordered_map<std::wstring,IFile *>::iterator mapIter,boost::unordered_map<std::wstring,IFile *>::iterator mapIterEnd): _mapIter(mapIter) , _mapIterEnd(mapIterEnd), _isEmpty(false) {};

	bool _isEmpty;
	boost::unordered_map<std::wstring,IFile *>::iterator _mapIter;
	boost::unordered_map<std::wstring,IFile *>::iterator _mapIterEnd;
};


}}}