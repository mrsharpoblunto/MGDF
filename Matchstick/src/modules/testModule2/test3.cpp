#include "StdAfx.h"

#include <fstream>
#include "Test3.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {


Test3::~Test3(void)
{
	delete _textManager;
}

Test3::Test3(ISystem *system)
{
	_system = system;
	_inited=false;
	_textManager = new TextManager(system);
	_stateBuffer.Pending()->AddLine(WHITE,"Load/Save Tests");
	_stateBuffer.Pending()->AddLine(WHITE,"");
	_testState = 0;
}

bool Test3::LoadModule(const char *saveDataFolder,const char *workingFolder,const IModuleInitialiser *init)
{
	_workingFolder = workingFolder;
	std::string saveFile(saveDataFolder);
	saveFile+="currentState.txt";
	std::ifstream in(saveFile.c_str(),std::ios::in);
	if (in.fail()) 
	{
		_testState = 3;
	}
	else
	{
		in >> _testState;
		in.close();
		if (_testState!=2) _testState = 3;
	}

	_stateBuffer.Pending()->AddLine(WHITE,"Passing parameters via module initialisers");
	_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
	_stateBuffer.Pending()->AddLine(WHITE,"Save game state");
	_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
	_stateBuffer.Pending()->AddLine(WHITE,"Load game state");
	return true;
}

bool Test3::NewModule(const char *workingFolder, const IModuleInitialiser *init)
{
	_stateBuffer.Pending()->AddLine(WHITE,"Passing parameters via module initialisers");
	if (init!=NULL && init->GetParam("caller")!=NULL && strcmp(init->GetParam("caller"),"Test2")==0)
	{
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
	}
	_workingFolder = workingFolder;
	return true;
}

bool Test3::Dispose(void)
{
	delete this;
	return true;
}

bool Test3::UpdateScene(double elapsedTime)
{
	if (_system->GetInput()->IsKeyPress(KEY_ESCAPE))
	{
		_system->ShutDown();
	}

	if (_testState==0)
	{
		_system->RemoveSave("testsave");
		_stateBuffer.Pending()->AddLine(WHITE,"Save game state");
		_system->QueueSaveGameState("testsave");
	}
	else if (_testState==1)
	{
		if (_system->GetSaves()->Size()==1 && strcmp(_system->GetSaves()->Get(0),"testsave")==0)
		{
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Load game state");
			_system->QueueLoadGameState("testsave");
		}
		else 
		{
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_testState = 999;
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
	}
	else if (_testState==2)
	{
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_testState = 4;
	}
	else if (_testState==3)
	{
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
		_testState = 999;
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==4)
	{
		_stateBuffer.Pending()->AddLine(WHITE,"Testing custom VFS archive handler registration");
		
		bool success = false;
		IFile *file = _system->GetVFS()->GetFile("/content/test.fakearchive/testfile.txt");
		if (file!=NULL)
		{
			file->OpenFile();
			char *data = new char[file->GetSize()];
			file->Read(data,file->GetSize());
			success = strncmp(data,"hello world",file->GetSize())==0;
			delete data;
		}

		if (!success)
		{
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_testState = 999;
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
		else
		{
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
			_testState++;
		}
	}
	else if (_testState==5)
	{
		_stateBuffer.Pending()->AddLine(WHITE,"Press [ALT]+[ENTER] to toggle fullscreen/windowed mode. Then press [Y/N] if this works correctly");
		_testState++;
	}
	else if (_testState==6 && _system->GetInput()->IsKeyPress(KEY_Y))
	{
		++_testState;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press [ALT]+[F12] to toggle the system information overlay. Then press [Y/N] if this works correctly");
	}
	else if (_testState==6 && _system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =999;
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==7 && _system->GetInput()->IsKeyPress(KEY_Y))
	{
		_testState =999;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");	}
	else if (_testState==7 && _system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =999;
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==999 && _system->GetInput()->IsKeyPress(KEY_RETURN))
	{
		//return to the original test module
		_system->QueuePopModules(1);
	}
	_stateBuffer.Flip();
	return true;
}

bool Test3::DrawScene(double alpha)
{
	boost::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
	if (state)
	{
		_textManager->SetState(state);
		_textManager->DrawText();
	}
	return true;
}

bool Test3::DeviceReset()
{
	_textManager->OnResetDevice();
	return true;
}

bool Test3::SetDeviceState()
{
	return true;
}

bool Test3::DeviceLost()
{	
	_textManager->OnLostDevice();
	return true;
}

bool Test3::CheckDeviceCaps() {
	return true;
}

bool Test3::Suspend()
{
	return true;
}

bool Test3::Resume()
{
	return true;
}

bool Test3::SaveModule(const char *saveDataFolder)
{
	_testState = 1;
	std::string saveFile(saveDataFolder);
	saveFile+="currentState.txt";
	std::ofstream out(saveFile.c_str(),std::ios::out);
	out << 2;
	out.close();
	return true;
}

void Test3::Panic()
{
}

const char * Test3::GetLastError()
{
	return _lastError.c_str();
}

}}