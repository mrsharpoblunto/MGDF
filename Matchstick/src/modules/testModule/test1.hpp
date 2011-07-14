#pragma once

#include <time.h>
#include "d3d9.h"
#include <MGDF/MGDF.hpp>

#include "Module.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

class Test1: public TestModule 
{
public:
	virtual ~Test1(void);
	Test1();

	virtual void Update(ISystem *system,TextManagerState *state);
	virtual void LoadModule(const wchar_t *,const wchar_t *,TextManagerState *state);
	virtual void SaveModule(const wchar_t *,TextManagerState *state);

	virtual TestModule *NextTestModule();
private:
	int _testState;
	time_t _time;
};

}}
