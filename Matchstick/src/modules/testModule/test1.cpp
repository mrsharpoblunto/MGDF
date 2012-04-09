#include "StdAfx.h"

#include "Test1.hpp"
#include "Test2.hpp"
#include <fstream>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

Test1::~Test1(void)
{
}

Test1::Test1()
{
	_testState = 0;
}

TestModule *Test1::NextTestModule()
{
	if (_testState==11) 
	{
		return new Test2();
	}
	else 
	{
		return NULL;
	}
}

void Test1::Update(ISystem *system,TextManagerState *state)
{
	if (system->GetInput()->IsKeyPress(KEY_ESCAPE))
	{
		system->ShutDown();
	}

	if (_testState==10)
	{
		system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(
			system->GetInput()->GetGamepads()->Get(0)->GetRightTrigger()*257,
			system->GetInput()->GetGamepads()->Get(0)->GetRightTrigger()*257
		);
	}

	time_t now = time(NULL);

	if (_testState==0)
	{
		_testState++;
		state->AddLine(WHITE,"InputManager Tests");
		state->AddLine(WHITE,"");
		state->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==1 && system->GetInput()->IsKeyPress(KEY_RETURN))
	{
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Press and hold [UP ARROW] key for at least one second");
		_time = time(NULL);
	}
	else if (_testState==2 && system->GetInput()->IsKeyDown(KEY_UP) && now>_time+1)
	{
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Now release the [UP ARROW] key");
	}
	else if (_testState==3 && system->GetInput()->IsKeyUp(KEY_UP))
	{
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Now click the left mouse button");
	}
	else if (_testState==4 && system->GetInput()->IsButtonClicked(MOUSE_LEFT))
	{
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Now move the mouse up");
	}
	else if (_testState==5 && system->GetInput()->GetMouseMovementY()<0)
	{
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");
		if (system->GetInput()->GetGamepads()->Get(0)->IsConnected()) {
			_testState++;
			state->AddLine(WHITE,"Xbox 360 controller detected");
		}
		else {
			state->AddLine(WHITE,"Plug in an xbox 360 controller or press [S] to skip controller tests");
		}
	}
	else if (_testState==6 && system->GetInput()->GetGamepads()->Get(0)->IsConnected())
	{
		_testState++;
		state->AddLine(WHITE,"Xbox 360 controller detected");
	}
	else if (_testState==6 && system->GetInput()->IsKeyPress(KEY_S))
	{
		_testState = 11;//skip past controller tests.
	}
	else if (_testState==7) {
		_testState++;
		state->AddLine(WHITE,"Press the [A] button on controller 1");		
	}
	else if (_testState==8 && system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_A)) {
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");	
		state->AddLine(WHITE,"Pull the left trigger on controller 1");
	}
	else if (_testState==9 && system->GetInput()->GetGamepads()->Get(0)->GetLeftTrigger()==255) {
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");	
		state->AddLine(WHITE,"Pull the right trigger on controller 1, press [A] if the controller vibrates, [B] if it does not.");
	}
	else if (_testState==10 && system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_A)) {
		_testState++;
		state->SetStatus(GREEN,"[Test Passed]");	
		system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(0,0);
	}
	else if (_testState==10 && system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_B)) {
		_testState++;
		state->SetStatus(RED,"[Test Failed]");	
		system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(0,0);
	}
}


}}