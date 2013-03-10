#pragma once

#include <MGDF/MGDFErrorHandler.hpp>

namespace MGDF { namespace core {

/* 
defines the interface used by core components. Core components are used to add a layer in between the core engine 
and the various third party libraries used to provide the engine functionality e.g json parsing, audio, input etc..
*/
class ISystemComponent {
public:
	void SetComponentErrorHandler(IErrorHandler *errorHandler)
	{
		_errorHandler = errorHandler;
	}

	IErrorHandler *GetComponentErrorHandler() const
	{
		return _errorHandler;
	}

	virtual ~ISystemComponent(){};
protected:
	IErrorHandler *_errorHandler;

};

}}