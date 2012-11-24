#pragma once

#include <string>
#include <boost/unordered_map.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FakeFile : public MGDF::DisposeImpl<MGDF::IFile>
{
public:
	FakeFile(IFile *parent,const std::wstring &physicalFile,const std::wstring &name);
	FakeFile(FakeFile *parent,const std::wstring &name,void *data,size_t dataLength);//NULL data indicates a folder
	void AddChild(MGDF::IFile *file);
	virtual ~FakeFile(void);

	virtual void Dispose();
	virtual MGDF::IFile *GetParent() const;
	virtual MGDF::IFileIterator *GetIterator();
	virtual MGDF::IFile *GetDescendant(const wchar_t * query);
	virtual MGDF::IFile *GetFirstChild();
	virtual MGDF::IFile *GetLastChild();
	virtual MGDF::IFile *GetChild(const wchar_t *name);
	virtual size_t GetChildCount();
	virtual const wchar_t* GetLogicalPath();

	virtual bool IsOpen() const;
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual UINT32 Read(void* buffer,UINT32 length);
	virtual void SetPosition(INT64 pos);
	virtual INT64 GetPosition() const;
	virtual bool EndOfFile() const;
	virtual INT64 GetSize();

	virtual bool IsFolder() const;
	virtual bool IsArchive() const;
	virtual const wchar_t *GetArchiveName() const;
	virtual const wchar_t *GetPhysicalPath() const;
	virtual const wchar_t *GetName() const;
	virtual time_t GetLastWriteTime() const;
protected:
	boost::unordered_map<std::wstring,MGDF::IFile *> *_children;
	MGDF::IFile *_parent;
	std::wstring _logicalPath;
	std::wstring _name;
	std::wstring _physicalPath;
	
	size_t _dataLength;
	void *_data;
	INT32 _position;
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
	FakeFileIterator(boost::unordered_map<std::wstring,MGDF::IFile *>::iterator mapIter,boost::unordered_map<std::wstring,MGDF::IFile *>::iterator mapIterEnd): _mapIter(mapIter) , _mapIterEnd(mapIterEnd), _isEmpty(false) {};

	bool _isEmpty;
	boost::unordered_map<std::wstring,MGDF::IFile *>::iterator _mapIter;
	boost::unordered_map<std::wstring,MGDF::IFile *>::iterator _mapIterEnd;
};