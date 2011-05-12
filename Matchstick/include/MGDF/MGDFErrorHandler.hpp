#pragma once

namespace MGDF {

/**
this interface allows subscribers to set the lastError filed on the system and also allows them to shut down the system
*/
class IErrorHandler {
public:
	virtual void SetLastError(const char *sender, int code,const char *description)=0;
	virtual void FatalError(const char *sender,const char *message)=0;
};

}