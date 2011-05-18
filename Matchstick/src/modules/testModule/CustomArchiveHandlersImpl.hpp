#pragma once

#include <MGDF/MGDF.hpp>

class CustomArchiveHandlersImpl: public MGDF::DisposeImpl<MGDF::ICustomArchiveHandlers>
{
public:
	virtual ~CustomArchiveHandlersImpl(){}
	virtual bool GetHandlers(MGDF::IArchiveHandler **list,unsigned int *length,MGDF::ILogger *logger,MGDF::IErrorHandler *errorHandler) const;
	virtual void Dispose(void);
};