#pragma once

#include <time.h>
#include <MGDF/MGDF.hpp>

#include "Module.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

class Test2: public TestModule 
{
public:
	virtual ~Test2(void);
	Test2();

	virtual void Update(ISystem *system,TextManagerState *state);

	virtual TestModule *NextTestModule();
private:
	int _testState;
	int _x,_y;
	ISound *_sound;
	ISoundStream *_stream;
};

}}
