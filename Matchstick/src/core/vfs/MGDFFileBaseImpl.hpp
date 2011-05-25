#pragma once

#include <string>
#include <hash_map>
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
	virtual IFile *GetDescendant(const char * query);
	virtual IFile *GetFirstChild();
	virtual IFile *GetLastChild();
	virtual IFile *GetChild(const char *name);
	virtual unsigned int GetChildCount();
	virtual const char* GetLogicalPath();

	IFile *GetChildInternal(const char *name);
	void AddChild(IFile *newNode);
	void SetParent(IFile *file); 
protected:
	stdext::hash_map<std::string,IFile *> *_children;
	IFile *_parent;
	std::string _logicalPath;
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
	FileBaseImplIterator(stdext::hash_map<std::string,IFile *>::iterator mapIter,stdext::hash_map<std::string,IFile *>::iterator mapIterEnd): _mapIter(mapIter) , _mapIterEnd(mapIterEnd), _isEmpty(false) {};

	bool _isEmpty;
	stdext::hash_map<std::string,IFile *>::iterator _mapIter;
	stdext::hash_map<std::string,IFile *>::iterator _mapIterEnd;
};


}}}