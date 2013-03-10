#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF
{
namespace core
{
namespace tests
{

class MockErrorHandler: public IErrorHandler
{
public:
	MockErrorHandler() {}
	virtual ~MockErrorHandler() {}
	virtual void SetLastError( const char *sender, UINT32 code, const char *description ) {}
	virtual void FatalError( const char *sender, const char *message ) {}
};

}
}
}