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
	void FatalError( const char *sender, const char *message ) override {}
};

}
}
}