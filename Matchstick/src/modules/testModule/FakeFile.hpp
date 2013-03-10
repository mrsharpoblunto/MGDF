#pragma once

#include <string>
#include <map>
#include <MGDF/MGDFVirtualFileSystem.hpp>

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FakeFile : public MGDF::IFile
{
public:
	FakeFile(const std::wstring &name,const std::wstring &physicalFile,IFile *parent);
	FakeFile(const std::wstring &name,FakeFile *parent,void *data,size_t dataLength);//NULL data indicates a folder
	void AddChild(MGDF::IFile *file);
	virtual ~FakeFile(void);

	virtual MGDF::IFile *GetParent() const;
	virtual MGDF::IFile *GetChild(const wchar_t *name);
	virtual bool GetAllChildren(const MGDF::IFileFilter *filter,IFile **childBuffer,size_t *bufferLength);
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
	std::map<const wchar_t *,MGDF::IFile *> *_children;
	MGDF::IFile *_parent;
	std::wstring _logicalPath;
	std::wstring _name;
	std::wstring _physicalPath;
	
	size_t _dataLength;
	void *_data;
	INT32 _position;
	bool _isOpen;
};