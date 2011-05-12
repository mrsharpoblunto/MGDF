#include "StdAfx.h"

#include "Test1.hpp"
#include <fstream>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

Test1::~Test1(void)
{
	delete _textManager;
}

Test1::Test1(ISystem *system)
{
	_system = system;
	_inited=false;
	_textManager = new TextManager(system);
	_stateBuffer.Pending()->AddLine(WHITE,"MGDF functional test suite started");
	_testState = 0;
}

bool Test1::LoadModule(const char *saveDataFolder,const char *workingFolder,const IModuleInitialiser *init)
{
	_workingFolder = workingFolder;
	if (init->GetParam("boot")==NULL) _testState = 12;//when booting the initializer will always have a 'boot' param with a value 'true'
	return true;
}

bool Test1::NewModule(const char *workingFolder, const IModuleInitialiser *init)
{
	_workingFolder = workingFolder;

	return true;
}

bool Test1::Dispose(void)
{
	delete this;
	return true;
}

bool Test1::UpdateScene(double elapsedTime)
{
	if (_system->GetInput()->IsKeyPress(KEY_ESCAPE))
	{
		_system->ShutDown();
	}

	if (_testState==10)
	{
		_system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(
			_system->GetInput()->GetGamepads()->Get(0)->GetRightTrigger()*257,
			_system->GetInput()->GetGamepads()->Get(0)->GetRightTrigger()*257
		);
	}

	time_t now = time(NULL);

	if (_testState==0)
	{
		_testState++;
		_stateBuffer.Pending()->AddLine(WHITE,"InputManager Tests");
		_stateBuffer.Pending()->AddLine(WHITE,"");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==1 && _system->GetInput()->IsKeyPress(KEY_RETURN))
	{
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press and hold [UP ARROW] key for at least one second");
		_time = time(NULL);
	}
	else if (_testState==2 && _system->GetInput()->IsKeyDown(KEY_UP) && now>_time+1)
	{
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Now release the [UP ARROW] key");
	}
	else if (_testState==3 && _system->GetInput()->IsKeyUp(KEY_UP))
	{
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Now click the left mouse button");
	}
	else if (_testState==4 && _system->GetInput()->IsButtonClicked(MOUSE_LEFT))
	{
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Now move the mouse up");
	}
	else if (_testState==5 && _system->GetInput()->GetMouseMovementY()<0)
	{
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		if (_system->GetInput()->GetGamepads()->Get(0)->IsConnected()) {
			_testState++;
			_stateBuffer.Pending()->AddLine(WHITE,"Xbox 360 controller detected");
		}
		else {
			_stateBuffer.Pending()->AddLine(WHITE,"Plug in an xbox 360 controller or press [S] to skip controller tests");
		}
	}
	else if (_testState==6 && _system->GetInput()->GetGamepads()->Get(0)->IsConnected())
	{
		_testState++;
		_stateBuffer.Pending()->AddLine(WHITE,"Xbox 360 controller detected");
	}
	else if (_testState==6 && _system->GetInput()->IsKeyPress(KEY_S))
	{
		_testState = 11;//skip past controller tests.
	}
	else if (_testState==7) {
		_testState++;
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [A] button on controller 1");		
	}
	else if (_testState==8 && _system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_A)) {
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");	
		_stateBuffer.Pending()->AddLine(WHITE,"Pull the left trigger on controller 1");
	}
	else if (_testState==9 && _system->GetInput()->GetGamepads()->Get(0)->GetLeftTrigger()==255) {
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");	
		_stateBuffer.Pending()->AddLine(WHITE,"Pull the right trigger on controller 1, press [A] if the controller vibrates, [B] if it does not.");
	}
	else if (_testState==10 && _system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_A)) {
		_testState++;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");	
		_system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(0,0);
	}
	else if (_testState==10 && _system->GetInput()->GetGamepads()->Get(0)->IsButtonPress(GAMEPAD_B)) {
		_testState++;
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");	
		_system->GetInput()->GetGamepads()->Get(0)->SetVibrationSpeed(0,0);
	}
	else if (_testState==11)
	{
		//test pushing modules (this module runs sound manager tests, then is popped off so this module resumes execution.
		_system->QueuePushNewModule("testModule1.dll:Test2",NULL);
	}
	else if (_testState>=13 && _testState!=999)
	{
		_testState=999;
		_stateBuffer.Pending()->AddLine(WHITE,"Resume Module");
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");	
		_stateBuffer.Pending()->AddLine(WHITE,"Tests complete Press ESC to exit (Make sure after you exit you don't see any memory leaks!)");
	}

	_stateBuffer.Flip();
	return true;
}

bool Test1::DrawScene(double alpha)
{
	boost::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
	if (state)
	{
		_textManager->SetState(state);
		_textManager->DrawText();
	}
   return true;
}

bool Test1::DeviceReset()
{
	_textManager->OnResetDevice();
	return true;
}

bool Test1::SetDeviceState()
{
	return true;
}

bool Test1::DeviceLost()
{
	_textManager->OnLostDevice();
	return true;
}

bool Test1::CheckDeviceCaps() {
	return true;
}

bool Test1::Suspend()
{
	return true;
}

bool Test1::Resume()
{
	++_testState;
	return true;
}

bool Test1::SaveModule(const char *saveDataFolder)
{
	return true;
}

void Test1::Panic()
{
}

const char * Test1::GetLastError()
{
	return _lastError.c_str();
}

}}