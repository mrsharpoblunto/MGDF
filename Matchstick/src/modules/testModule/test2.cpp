#include "StdAfx.h"

#include "Test2.hpp"
#include "Test3.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

Test2::~Test2(void)
{
}

Test2::Test2()
{
	_testState = 0;
}

TestModule *Test2::NextTestModule()
{
	if (_testState==1000)
	{
		return new Test3();
	}
	else
	{
		return NULL;
	}
}

void Test2::Update(ISystem *system,TextManagerState *state)
{
	if (system->GetInput()->IsKeyPress(KEY_ESCAPE))
	{
		system->ShutDown();
	}

	if (_testState==0)
	{
		state->AddLine(WHITE,"");
		state->AddLine(WHITE,"SoundManager Tests");
		state->AddLine(WHITE,"");

		_testState++;
		state->AddLine(WHITE,"Checking SoundManager is initialized");
		if (system->GetSound()!=NULL) 
		{
			state->SetStatus(GREEN,"[Test Passed]");
		}
		else 
		{
			_testState =1000;
			state->SetStatus(RED,"[Test Failed]");
		}
	}
	else if (_testState==1)
	{
		state->AddLine(WHITE,"Loading sound chimes.wav");
		_sound = system->GetSound()->CreateSound(system->GetVFS()->GetFile(L"content/chimes.wav"),0);
		system->GetSound()->SetEnableAttenuation(true);
		if (_sound==NULL)
		{
			_testState =1000;
			state->SetStatus(RED,"[Test Failed]");
		}
		else 
		{
			++_testState;
			state->SetStatus(GREEN,"[Test Passed]");
		}
	}
	else if (_testState==2)
	{
		_sound->SetLooping(true);
		_sound->SetInnerRange(0);
		_sound->SetOuterRange(250);
		_sound->Play();
		state->AddLine(WHITE,"Is a sound playing? [Y/N]");
		++_testState;
	}
	else if (_testState==3 && system->GetInput()->IsKeyPress(KEY_Y))
	{
		++_testState;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Use arrow keys to change sounds position, press [Y/N] if the sound adjusts accordingly");
	}
	else if (_testState==3 && system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =1000;
		system->GetSound()->RemoveSound(_sound);
		state->SetStatus(RED,"[Test Failed]");
	}
	else if (_testState==4)
	{
		if (system->GetInput()->IsKeyDown(KEY_UP))
		{
			_sound->GetPosition()->y +=1;
		}
		if (system->GetInput()->IsKeyDown(KEY_DOWN))
		{
			_sound->GetPosition()->y -=1;
		}
		if (system->GetInput()->IsKeyDown(KEY_LEFT))
		{
			_sound->GetPosition()->x -=1;
		}
		if (system->GetInput()->IsKeyDown(KEY_RIGHT))
		{
			_sound->GetPosition()->x +=1;
		}

		if (system->GetInput()->IsKeyPress(KEY_Y))
		{
			++_testState;
			_sound->Stop();
			system->GetSound()->RemoveSound(_sound);
			state->SetStatus(GREEN,"[Test Passed]");
		}
		else if (system->GetInput()->IsKeyPress(KEY_N))
		{
			_testState =1000;
			_sound->Stop();
			system->GetSound()->RemoveSound(_sound);
			state->SetStatus(RED,"[Test Failed]");
		}
	}
	else if (_testState==5)
	{
		state->AddLine(WHITE,"Loading stream stream.ogg");
		_stream = system->GetSound()->CreateSoundStream(system->GetVFS()->GetFile(L"content/stream.ogg"));
		if (_stream==NULL)
		{
			_testState =1000;
			state->SetStatus(RED,"[Test Failed]");
		}
		else 
		{
			++_testState;
			state->SetStatus(GREEN,"[Test Passed]");
		}
		
	}
	else if (_testState==6)
	{
		++_testState;
		_stream->Play();
		state->AddLine(WHITE,"Playing stream, press [Y/N] if the stream is actually playing");
	}
	else if (_testState==7 && system->GetInput()->IsKeyPress(KEY_Y))
	{
		++_testState;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Use [P] to toggle pause/play, press [Y/N] if this is working.");
	}
	else if (_testState==7 && system->GetInput()->IsKeyPress(KEY_N))
	{
		_testState =1000;
		system->GetSound()->RemoveSoundStream(_stream);
		state->SetStatus(RED,"[Test Failed]");
	}
	else if (_testState==8)
	{
		if (system->GetInput()->IsKeyPress(KEY_Y))
		{
			_testState =1000;
			system->GetSound()->RemoveSoundStream(_stream);
			state->SetStatus(GREEN,"[Test Passed]");
		}
		else if (system->GetInput()->IsKeyPress(KEY_N))
		{
			_testState =1000;
			system->GetSound()->RemoveSoundStream(_stream);
			state->SetStatus(RED,"[Test Failed]");
		}
		else if (system->GetInput()->IsKeyPress(KEY_P))
		{
			if (_stream->IsPaused()) _stream->Play();
			else _stream->Pause();
		}
	}
}

}}