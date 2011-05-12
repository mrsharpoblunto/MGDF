#include "StdAfx.h"

#include "Test2.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

Test2::~Test2(void)
{
	delete _textManager;
}

Test2::Test2(ISystem *system)
{
	_system = system;
	_inited=false;
	_textManager = new TextManager(system);
	_stateBuffer.Pending()->AddLine(WHITE,"SoundManager Tests");
	_stateBuffer.Pending()->AddLine(WHITE,"");
	_testState = 0;
}

bool Test2::LoadModule(const char *saveDataFolder,const char *workingFolder,const IModuleInitialiser *init)
{
	_workingFolder = workingFolder;
	return true;
}

bool Test2::NewModule(const char *workingFolder, const IModuleInitialiser *init)
{
	_workingFolder = workingFolder;

	return true;
}

bool Test2::Dispose(void)
{
	delete this;
	return true;
}

bool Test2::UpdateScene(double elapsedTime)
{
	if (_system->GetInput()->IsKeyPress(KEY_ESCAPE))
	{
		_system->ShutDown();
	}

	time_t now = time(NULL);

	if (_testState==0)
	{
		_testState++;
		_stateBuffer.Pending()->AddLine(WHITE,"Checking SoundManager is initialized");
		if (_system->GetSound()!=NULL) 
		{
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		}
		else 
		{
			_testState =999;
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
	}
	else if (_testState==1)
	{
		_stateBuffer.Pending()->AddLine(WHITE,"Loading sound chimes.wav");
		_sound = _system->GetSound()->CreateSound(_system->GetVFS()->GetFile("content/chimes.wav"),0);
		_system->GetSound()->SetEnableAttenuation(true);
		if (_sound==NULL)
		{
			_testState =999;
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
		else 
		{
			++_testState;
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		}
	}
	else if (_testState==2)
	{
		_sound->SetLooping(true);
		_sound->SetInnerRange(0);
		_sound->SetOuterRange(250);
		_sound->Play();
		_stateBuffer.Pending()->AddLine(WHITE,"Is a sound playing? [Y/N]");
		++_testState;
	}
	else if (_testState==3 && _system->GetInput()->IsKeyPress(KEY_Y))
	{
		++_testState;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Use arrow keys to change sounds position, press [Y/N] if the sound adjusts accordingly");
	}
	else if (_testState==3 && _system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =999;
		_system->GetSound()->RemoveSound(_sound);
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==4)
	{
		if (_system->GetInput()->IsKeyDown(KEY_UP))
		{
			_sound->GetPosition()->Y +=1;
		}
		if (_system->GetInput()->IsKeyDown(KEY_DOWN))
		{
			_sound->GetPosition()->Y -=1;
		}
		if (_system->GetInput()->IsKeyDown(KEY_LEFT))
		{
			_sound->GetPosition()->X -=1;
		}
		if (_system->GetInput()->IsKeyDown(KEY_RIGHT))
		{
			_sound->GetPosition()->X +=1;
		}

		if (_system->GetInput()->IsKeyPress(KEY_Y))
		{
			++_testState;
			_sound->Stop();
			_system->GetSound()->RemoveSound(_sound);
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		}
		else if (_system->GetInput()->IsKeyPress(KEY_N))
		{
			_testState =999;
			_sound->Stop();
			_system->GetSound()->RemoveSound(_sound);
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
	}
	else if (_testState==5)
	{
		_stateBuffer.Pending()->AddLine(WHITE,"Loading stream stream.ogg");
		_stream = _system->GetSound()->CreateSoundStream(_system->GetVFS()->GetFile("content/stream.ogg"));
		if (_stream==NULL)
		{
			_testState =999;
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
		else 
		{
			++_testState;
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		}
		
	}
	else if (_testState==6)
	{
		++_testState;
		_stream->Play();
		_stateBuffer.Pending()->AddLine(WHITE,"Playing stream, press [Y/N] if the stream is actually playing");
	}
	else if (_testState==7 && _system->GetInput()->IsKeyPress(KEY_Y))
	{
		++_testState;
		_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Use [P] to toggle pause/play, press [Y/N] if this is working.");
	}
	else if (_testState==7 && _system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =999;
		_system->GetSound()->RemoveSoundStream(_stream);
		_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
		_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
	}
	else if (_testState==8)
	{
		if (_system->GetInput()->IsKeyPress(KEY_Y))
		{
			_testState =999;
			_system->GetSound()->RemoveSoundStream(_stream);
			_stateBuffer.Pending()->SetStatus(GREEN,"[Test Passed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
		else if (_system->GetInput()->IsKeyPress(KEY_N))
		{
			_testState =999;
			_system->GetSound()->RemoveSoundStream(_stream);
			_stateBuffer.Pending()->SetStatus(RED,"[Test Failed]");
			_stateBuffer.Pending()->AddLine(WHITE,"Press the [ENTER] key");
		}
		else if (_system->GetInput()->IsKeyPress(KEY_P))
		{
			if (_stream->IsPaused()) _stream->Play();
			else _stream->Pause();
		}
	}
	else if (_testState==999 && _system->GetInput()->IsKeyPress(KEY_RETURN))
	{
		//load up a module from another dll
		IModuleInitialiser *init = _system->CreateModuleInitialiser();
		init->AddParam("caller","Test2");
		_system->QueueSwapTopModule("testModule2.dll:Test3",init);
	}
	_stateBuffer.Flip();
	return true;
}

bool Test2::DrawScene(double alpha)
{
	boost::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
	if (state)
	{
		_textManager->SetState(state);
		_textManager->DrawText();
	}
	return true;
}

bool Test2::DeviceReset()
{
	_textManager->OnResetDevice();
	return true;
}

bool Test2::SetDeviceState()
{
	return true;
}

bool Test2::DeviceLost()
{
	_textManager->OnLostDevice();
	return true;
}

bool Test2::CheckDeviceCaps() {
	return true;
}

bool Test2::Suspend()
{
	return true;
}

bool Test2::Resume()
{
	return true;
}

bool Test2::SaveModule(const char *saveDataFolder)
{
	return true;
}

void Test2::Panic()
{
}

const char * Test2::GetLastError()
{
	return _lastError.c_str();
}

}}