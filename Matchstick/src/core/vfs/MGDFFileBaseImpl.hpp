#pragma once

#include <string>
#include <map>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF { namespace core { namespace vfs {

struct WCharCmp
{
   bool operator()(const wchar_t *a, const wchar_t *b)
   {
	   return std::wcscmp(a, b) < 0;
   }
};

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FileBaseImpl : public IFile
{
public:
	FileBaseImpl(IFile *parent);
	virtual ~FileBaseImpl();

	virtual IFile *GetParent() const { return _parent; }
	virtual IFile *GetChild(const wchar_t *name);
	virtual size_t GetChildCount() { return _children ? _children->size() : 0; }
	virtual bool GetAllChildren(const IFileFilter *filter,IFile **childBuffer,size_t *bufferLength);
	virtual bool IsArchive() const { return false; }

	virtual const wchar_t* GetLogicalPath();
	virtual time_t GetLastWriteTime() const;

	IFile *GetChildInternal(const wchar_t *name);
	void AddChild(IFile *newNode);
	void SetParent(IFile *file); 
protected:
	std::map<const wchar_t *,IFile *,WCharCmp> *_children;
	IFile *_parent;
	std::wstring _logicalPath;
};


}}}