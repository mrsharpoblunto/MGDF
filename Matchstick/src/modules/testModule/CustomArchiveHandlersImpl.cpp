#include "StdAfx.h"

#include "CustomArchiveHandlersImpl.hpp"
#include "FakeArchiveHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

bool CustomArchiveHandlersImpl::GetHandlers(MGDF::IArchiveHandler **list,UINT32 *length,MGDF::ILogger *logger,MGDF::IErrorHandler *errorHandler) const
{
	if (*length==0)
	{
		*length = 1;
		return false;
	}
	else
	{
		MGDF::IArchiveHandler *handler = new FakeArchiveHandler(logger,errorHandler);
		list[0] = handler;
		return true;
	}
}

void CustomArchiveHandlersImpl::Dispose(void)
{
	delete this;
}