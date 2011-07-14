#pragma once

#include <string>
#include <hash_map>
#include <MGDF/MGDFVirtualFileSystem.hpp>

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FakeFile : public MGDF::DisposeImpl<MGDF::IFile>
{
public:
	FakeFile(IFile *parent,std::wstring physicalFile,std::wstring name);
	FakeFile(FakeFile *parent,std::wstring name,void *data,int dataLength);//NULL data indicates a folder
	void AddChild(MGDF::IFile *file);
	virtual ~FakeFile(void);

	virtual void Dispose();
	virtual MGDF::IFile *GetParent() const;
	virtual MGDF::IFileIterator *GetIterator();
	virtual MGDF::IFile *GetDescendant(const wchar_t * query);
	virtual MGDF::IFile *GetFirstChild();
	virtual MGDF::IFile *GetLastChild();
	virtual MGDF::IFile *GetChild(const wchar_t *name);
	virtual unsigned int GetChildCount();
	virtual const wchar_t* GetLogicalPath();

	virtual bool IsOpen() const;
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual int Read(void* buffer,int length);
	virtual void SetPosition(unsigned long pos);
	virtual unsigned long GetPosition() const;
	virtual bool EndOfFile() const;
	virtual unsigned long GetSize();

	virtual bool IsFolder() const;
	virtual bool IsArchive() const;
	virtual const wchar_t *GetArchiveName() const;
	virtual const wchar_t *GetPhysicalPath() const;
	virtual const wchar_t *GetName() const;
protected:
	stdext::hash_map<std::wstring,MGDF::IFile *> *_children;
	MGDF::IFile *_parent;
	std::wstring _logicalPath;
	std::wstring _name;
	std::wstring _physicalPath;
	
	int _dataLength;
	void *_data;
	int _position;
	bool _isOpen;
};

class FakeFileIterator: public MGDF::DisposeImpl<MGDF::IFileIterator>
{ 
	friend class FakeFile;
public:
	virtual ~FakeFileIterator(){};
	virtual MGDF::IFile *Current() const;
	virtual MGDF::IFile *Next();
	virtual bool HasNext() const;
	virtual void Dispose();
private:
	FakeFileIterator() : _isEmpty(true) {};
	FakeFileIterator(stdext::hash_map<std::wstring,MGDF::IFile *>::iterator mapIter,stdext::hash_map<std::wstring,MGDF::IFile *>::iterator mapIterEnd): _mapIter(mapIter) , _mapIterEnd(mapIterEnd), _isEmpty(false) {};

	bool _isEmpty;
	stdext::hash_map<std::wstring,MGDF::IFile *>::iterator _mapIter;
	stdext::hash_map<std::wstring,MGDF::IFile *>::iterator _mapIterEnd;
};