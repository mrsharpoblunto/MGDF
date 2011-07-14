#include "StdAfx.h"

#include "ZipFolderImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs { namespace zip {

ZipFolderImpl::~ZipFolderImpl() 
{
	//only if no other zip files are referencing the handler do we delete it
	_handler->DecRefCount();
	if (_handler->GetRefCount()==0) {
		delete _handler;
	}
}

bool ZipFolderImpl::IsArchive() const
{
	return true;
}


const wchar_t * ZipFolderImpl::GetArchiveName() const
{
	return _handler->GetArchiveRoot()->GetName();
}

}}}}