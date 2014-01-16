#include "StdAfx.h"

#include "Test2.hpp"
#include "Test3.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

Test2::~Test2( void )
{
}

Test2::Test2()
{
	_testState = 0;
}

TestModule *Test2::NextTestModule()
{
	if ( _testState == 1000 ) {
		return new Test3();
	} else {
		return NULL;
	}
}

void Test2::Update( ISimHost *host, TextManagerState *state )
{
	if ( host->GetInput()->IsKeyPress( VK_ESCAPE ) ) {
		host->ShutDown();
	}

	if ( _testState == 0 ) {
		state->AddLine( "" );
		state->AddLine( "SoundManager Tests" );
		state->AddLine( "" );

		_testState++;
		state->AddLine( "Checking SoundManager is initialized" );
		if ( host->GetSound() != NULL ) {
			state->SetStatus( GREEN, "[Test Passed]" );
		} else {
			_testState = 1000;
			state->SetStatus( RED, "[Test Failed]" );
		}
	} else if ( _testState == 1 ) {
		state->AddLine( "Loading sound chimes.wav" );
		host->GetSound()->SetEnableAttenuation( true );
		if ( MGDF_OK != host->GetSound()->CreateSound( host->GetVFS()->GetFile( L"chimes.wav" ), 0, &_sound ) ) {
			_testState = 1000;
			state->SetStatus( RED, "[Test Failed]" );
		} else {
			++_testState;
			state->SetStatus( GREEN, "[Test Passed]" );
		}
	} else if ( _testState == 2 ) {
		_sound->SetLooping( true );
		_sound->SetInnerRange( 0 );
		_sound->SetOuterRange( 250 );
		_sound->Play();
		state->AddLine( "Is a sound playing? [Y/N]" );
		++_testState;
	} else if ( _testState == 3 && host->GetInput()->IsKeyPress( 'Y' ) ) {
		++_testState;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Use arrow keys to change sounds position, press [Y/N] if the sound adjusts accordingly" );
	} else if ( _testState == 3 && host->GetInput()->IsKeyPress( 'N' ) ) {
		_testState = 1000;
		_sound->Dispose();
		state->SetStatus( RED, "[Test Failed]" );
	} else if ( _testState == 4 ) {
		if ( host->GetInput()->IsKeyDown( VK_UP ) ) {
			_sound->GetPosition()->y += 1;
		}
		if ( host->GetInput()->IsKeyDown( VK_DOWN ) ) {
			_sound->GetPosition()->y -= 1;
		}
		if ( host->GetInput()->IsKeyDown( VK_LEFT ) ) {
			_sound->GetPosition()->x -= 1;
		}
		if ( host->GetInput()->IsKeyDown( VK_RIGHT ) ) {
			_sound->GetPosition()->x += 1;
		}

		if ( host->GetInput()->IsKeyPress( 'Y' ) ) {
			++_testState;
			_sound->Stop();
			_sound->Dispose();
			state->SetStatus( GREEN, "[Test Passed]" );
		} else if ( host->GetInput()->IsKeyPress( 'N' ) ) {
			_testState = 1000;
			_sound->Stop();
			_sound->Dispose();
			state->SetStatus( RED, "[Test Failed]" );
		}
	} else if ( _testState == 5 ) {
		state->AddLine( "Loading stream stream.ogg" );
		if ( MGDF_OK != host->GetSound()->CreateSoundStream( host->GetVFS()->GetFile( L"Stream.ogg" ), &_stream ) ) {
			_testState = 1000;
			state->SetStatus( RED, "[Test Failed]" );
		} else {
			++_testState;
			state->SetStatus( GREEN, "[Test Passed]" );
		}

	} else if ( _testState == 6 ) {
		++_testState;
		if ( MGDF_OK != _stream->Play() ) {
			_testState = 1000;
			state->SetStatus( RED, "[Test Failed]" );
		}
		else {
			state->AddLine( "Playing stream, press [Y/N] if the stream is actually playing" );
		}
	} else if ( _testState == 7 && host->GetInput()->IsKeyPress( 'Y' ) ) {
		++_testState;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Use [P] to toggle pause/play, press [Y/N] if this is working." );
	} else if ( _testState == 7 && host->GetInput()->IsKeyPress( 'N' ) ) {
		_testState = 1000;
		_stream->Dispose();
		state->SetStatus( RED, "[Test Failed]" );
	} else if ( _testState == 8 ) {
		if ( host->GetInput()->IsKeyPress( 'Y' ) ) {
			_testState = 1000;
			_stream->Dispose();
			state->SetStatus( GREEN, "[Test Passed]" );
		} else if ( host->GetInput()->IsKeyPress( 'N' ) ) {
			_testState = 1000;
			_stream->Dispose();
			state->SetStatus( RED, "[Test Failed]" );
		} else if ( host->GetInput()->IsKeyPress( 'P' ) ) {
			if ( _stream->IsPaused() ) {
				if ( MGDF_OK != _stream->Play() ) {
					_testState = 1000;
					state->SetStatus( RED, "[Test Failed]" );
				}
			}
			else _stream->Pause();
		}
	}
}

}
}