#pragma once

#include <MGDF/MGDFDisposable.hpp>

namespace MGDF {

class IPerformanceCounter: public IDisposable {
public:
	virtual const char *GetName() const=0;
	virtual void Begin()=0;
	virtual void End()=0;
};

}