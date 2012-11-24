#pragma once

#include <MGDF/MGDF.hpp>

#include "Module.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

/**
this class acts as a command console to the lua system interfaces
*/
class Test3: public TestModule 
{
public:
	virtual ~Test3(void);
	Test3();

	virtual void Update(ISystem *system,TextManagerState *state);

	virtual TestModule *NextTestModule();
private:
	INT32 _testState;

};

}}
