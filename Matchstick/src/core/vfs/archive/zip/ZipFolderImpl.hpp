#pragma once

#include "ZipArchive.hpp"
#include "../../MGDFFolderBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs { namespace zip {

/**
implementation of a folder in a zipped archive
these files are essentially flyweight objects, all the zip functionality is encapsulated in the ZipArchiveHandler class
*/
class ZipFolderImpl: public FolderBaseImpl
{
	friend class ZipArchive;//allow the handler to create instances
public:
	virtual ~ZipFolderImpl();
	virtual bool IsArchive() const;
	virtual const wchar_t *GetArchiveName() const;
private:
	ZipArchive *_handler;

	ZipFolderImpl(ZipArchive *handler,const std::wstring &name): FolderBaseImpl(name,handler->GetArchiveRoot()->GetPhysicalPath())
	{
		_handler = handler;
		_handler->IncRefCount();
	}
};

}}}}