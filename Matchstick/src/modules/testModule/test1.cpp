#include "StdAfx.h"

#include "Test1.hpp"
#include "Test2.hpp"
#include <fstream>


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

Test1::~Test1( void )
{
}

Test1::Test1()
{
	_testState = 0;
}

TestModule *Test1::NextTestModule()
{
	if ( _testState == 12 ) {
		return new Test2();
	} else {
		return nullptr;
	}
}

void Test1::Update( ISimHost *host, TextManagerState *state )
{
	if ( host->GetInput()->IsKeyPress( VK_ESCAPE ) ) {
		host->ShutDown();
	}

	if ( _testState == 11 ) {
		host->GetInput()->GetGamepads()->Get( 0 )->SetVibrationSpeed(
		    host->GetInput()->GetGamepads()->Get( 0 )->GetRightTrigger() * 257,
		    host->GetInput()->GetGamepads()->Get( 0 )->GetRightTrigger() * 257
		);
	}

	time_t now = time( nullptr );

	if ( _testState == 0 ) {
		_testState++;
		state->AddLine( "InputManager Tests" );
		state->AddLine( "" );
		state->AddLine( "Press the [ENTER] key" );
	} else if ( _testState == 1 && host->GetInput()->IsKeyPress( VK_RETURN ) ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Press and hold [UP ARROW] key for at least one second" );
	} else if ( _testState == 2 && host->GetInput()->IsKeyDown( VK_UP ) ) {
		_time = host->GetTimer()->GetCurrentTimeTicks();
		_testState++;
	} else if ( _testState == 3 ) {
		if ( host->GetInput()->IsKeyDown( VK_UP ) ) {
			if ( host->GetTimer()->ConvertDifferenceToSeconds( host->GetTimer()->GetCurrentTimeTicks(), _time ) > 1 ) {
				_testState++;
				state->SetStatus( GREEN, "[Test Passed]" );
				state->AddLine( "Now release the [UP ARROW] key" );
			}
		} else {
			_testState--;
		}
	} else if ( _testState == 4 && host->GetInput()->IsKeyUp( VK_UP ) ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Now click the left mouse button" );
	} else if ( _testState == 5 && host->GetInput()->IsButtonClicked( MOUSE_LEFT ) ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Now move the mouse up" );
	} else if ( _testState == 6 && host->GetInput()->GetMouseDY() < 0 ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		if ( host->GetInput()->GetGamepads()->Get( 0 )->IsConnected() ) {
			_testState++;
			state->AddLine( "Xbox 360 controller detected" );
		} else {
			state->AddLine( "Plug in an xbox 360 controller or press [S] to skip controller tests" );
		}
	} else if ( _testState == 7 && host->GetInput()->GetGamepads()->Get( 0 )->IsConnected() ) {
		_testState++;
		state->AddLine( "Xbox 360 controller detected" );
	} else if ( _testState == 7 && host->GetInput()->IsKeyPress( 'S' ) ) {
		_testState = 12;//skip past controller tests.
	} else if ( _testState == 8 ) {
		_testState++;
		state->AddLine( "Press the [A] button on controller 1" );
	} else if ( _testState == 9 && host->GetInput()->GetGamepads()->Get( 0 )->IsButtonPress( GAMEPAD_A ) ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Pull the left trigger on controller 1" );
	} else if ( _testState == 10 && host->GetInput()->GetGamepads()->Get( 0 )->GetLeftTrigger() == 255 ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		state->AddLine( "Pull the right trigger on controller 1, press [A] if the controller vibrates, [B] if it does not." );
	} else if ( _testState == 11 && host->GetInput()->GetGamepads()->Get( 0 )->IsButtonPress( GAMEPAD_A ) ) {
		_testState++;
		state->SetStatus( GREEN, "[Test Passed]" );
		host->GetInput()->GetGamepads()->Get( 0 )->SetVibrationSpeed( 0, 0 );
	} else if ( _testState == 11 && host->GetInput()->GetGamepads()->Get( 0 )->IsButtonPress( GAMEPAD_B ) ) {
		_testState++;
		state->SetStatus( RED, "[Test Failed]" );
		host->GetInput()->GetGamepads()->Get( 0 )->SetVibrationSpeed( 0, 0 );
	}
}


}
}