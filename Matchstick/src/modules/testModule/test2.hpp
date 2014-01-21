#pragma once

#include <time.h>
#include <MGDF/MGDF.hpp>

#include "Module.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

class Test2: public TestModule
{
public:
	virtual ~Test2( void );
	Test2();

	void Update( ISimHost *host, TextManagerState *state ) override final;

	TestModule *NextTestModule() override final;
private:
	INT32 _testState;
	INT32 _x, _y;
	ISound *_sound;
	ISoundStream *_stream;
};

}
}
