#include "StdAfx.h"
 
#include "MGDFApp.hpp"
#include "core.impl/MGDFSystemBuilder.hpp"
#include "core.impl/MGDFParameterConstants.hpp"
#include <boost/lexical_cast.hpp>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

DEFINE_SINGLETON(MGDFApp)

MGDFApp::MGDFApp(HINSTANCE hInstance) : D3DAppFramework(hInstance) 
{
	_font=nullptr;
	_system = nullptr;
	_initialized = false;
}

MGDFApp::~MGDFApp() 
{
	SAFE_RELEASE(_font);
}

//allows the app to read from system configuration preferences when setting up d3d
void MGDFApp::SetSystem(System *system)
{
	_system = system;
	_system->AddShutDownCallback([this]()
	{
		_internalShutDown = true;
		PostMessage(MGDFApp::Instance()._window,WM_CLOSE,0,0);
	});
}

void MGDFApp::InitDirect3D(const std::string &caption,WNDPROC windowProcedure)
{
	D3DAppFramework::InitDirect3D(caption,windowProcedure);
	_system->SetD3DDevice(_d3dDevice);//allow the system to pass the d3d object to the modules

	IFW1Factory *factory;
	if (FAILED(FW1CreateFactory(FW1_VERSION, &factory)))
	{
		FatalError("unable to create MGDF system font factory");
	}
	
	if (FAILED(factory->CreateFontWrapper(_d3dDevice, L"Arial", &_font)))
	{
		FatalError("unable to create MGDF system font");
	}

	SAFE_RELEASE(factory);
}

void MGDFApp::UpdateScene(double simulationTime) 
{
	LARGE_INTEGER simulationStart = _timer.GetCurrentTimeTicks();

	if (!_initialized)
	{	
		_system->Initialize();
		_initialized = true;
	}

	//execute one frame of game logic as per the current module
	_system->UpdateScene(simulationTime,&_stats,_statsMutex);

	LARGE_INTEGER simulationEnd = _timer.GetCurrentTimeTicks();

	boost::mutex::scoped_lock lock(_statsMutex);
	_stats.AppendActiveSimTime(_timer.ConvertDifferenceToSeconds(simulationEnd,simulationStart) - _stats.SimInputTime() - _stats.SimAudioTime());
}

void MGDFApp::OnInitD3D(ID3D11Device *device,IDXGIAdapter1 *adapter)
{
	_system->CreateGraphicsImpl(device,adapter);
}

bool MGDFApp::IsBackBufferChangePending()
{
	return _system->GetGraphicsImpl()->IsBackBufferChangePending();
}

void MGDFApp::OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *swapDesc,BOOL *doFullScreen) 
{
	_system->GetGraphicsImpl()->OnResetSwapChain(swapDesc,doFullScreen);
}

void MGDFApp::OnBackBufferChanged(ID3D11Texture2D *backBuffer)
{
	_system->GetGraphicsImpl()->SetBackBuffer(backBuffer);
	_system->BackBufferChanged();
}

void MGDFApp::DrawScene(double alpha) 
{
	_system->DrawScene(alpha);//render as per the current active module

	if (_drawSystemOverlay) {
		DrawSystemOverlay();
	}
}

void MGDFApp::DrawSystemOverlay()
{
	std::wstring information;
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		std::string info =_system->GetSystemInformation(&_stats);
		information.assign(info.begin(),info.end());
	}

	_font->DrawString(
		_immediateContext,
		information.c_str(),// String
		14.0f,// Font size
		0.0f,// X position
		0.0f,// Y position
		0xFFFFFFFF,// Text color
		0// Flags (for example FW1_RESTORESTATE to keep context states unchanged)
	);
}

void MGDFApp::FatalError(const std::string &message)
{
	_system->FatalError("MGDF",message);
}

void MGDFApp::ExternalClose()
{
	_system->QueueShutDown();
}

}}