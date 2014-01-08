#pragma once

#include <MGDF/MGDF.hpp>

#include "Module.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

/**
this class acts as a command console to the lua  interfaces
*/
class Test3: public TestModule
{
public:
	virtual ~Test3( void );
	Test3();

	void Update( ISimHost *host, TextManagerState *state )  override;

	TestModule *NextTestModule() override;
private:
	INT32 _testState;

};

}
}
