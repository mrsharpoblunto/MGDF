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
	_font=NULL;
	_vBuffer=NULL;
	_system = NULL;
	_initialized = false;
}

MGDFApp::~MGDFApp() 
{
	if(_font!=NULL){
      _font->Release();
      _font=NULL;
   }
	if(_vBuffer!=NULL){
      _vBuffer->Release();
      _vBuffer=NULL;
   }
}

//allows the app to read from system configuration preferences when setting up d3d
void MGDFApp::SetSystem(System *system)
{
	_system = system;
	_system->AddShutDownCallback(boost::bind(&MGDFApp::ShutDownCallBack,this));
}

void MGDFApp::InitDirect3D(const std::string &caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen)
{
	D3DAppFramework::InitDirect3D(caption,windowProcedure,devType,requestedVP,canToggleFullScreen);
	_system->SetD3DDevice(_d3dDevice);//allow the system to pass the d3d object to the modules

	HRESULT hr=D3DXCreateFont(_d3dDevice,//D3D Device
                     16,               //Font height
                     0,                //Font width
                     FW_NORMAL,        //Font Weight
                     1,                //MipLevels
                     false,            //Italic
                     DEFAULT_CHARSET,  //CharSet
                     OUT_DEFAULT_PRECIS, //OutputPrecision
                     ANTIALIASED_QUALITY, //Quality
                     DEFAULT_PITCH|FF_DONTCARE,//PitchAndFamily
                     "Arial",          //pFacename,
                     &_font);         //ppFont

	if (FAILED(hr)) {
		FatalError("unable to create MGDF system font");
	}

	for (int i=0;i<6;++i) 
	{
		_vertices[i].X = 0.0f;
		_vertices[i].Y = 0.0f;
		_vertices[i].Z = 0.0f;
		_vertices[i].RHW = 1.0f;
		_vertices[i].COLOR = D3DCOLOR_XRGB(0,0,0);
	}

	_d3dDevice->CreateVertexBuffer(6*sizeof(CUSTOMVERTEX),
                               0,
                               CUSTOMFVF,
                               D3DPOOL_MANAGED,
                               &_vBuffer,
                               NULL);
}


bool MGDFApp::CheckDeviceCaps()
{
	return true;
}

void MGDFApp::OnLostDevice() 
{
	_font->OnLostDevice();
	_system->DeviceLost();
}

void MGDFApp::OnResetDevice() 
{
	_font->OnResetDevice();
	_system->DeviceReset();
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

bool MGDFApp::IsResetSwapChainPending()
{
	return _system->GetGraphicsImpl()->IsResetSwapChainPending();
}

void MGDFApp::OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *swapDesc) 
{
	_system->GetGraphicsImpl()->OnResetSwapChain(swapDesc);
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
	RECT font_rect;
	SetRect(&font_rect,0,0,100,100);

	std::string information;
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		information =_system->GetSystemInformation(&_stats);
		//TODO create vertex buffers for graph here too.
	}

	_font->DrawText(NULL,        //pSprite
		information.c_str(),  //pString
        -1,          //Count
        &font_rect,  //pRect
        DT_CALCRECT,//Format,
        0xFFFFFFFF); //Color

	if (_vertices[1].X != font_rect.right || _vertices[2].Y != font_rect.bottom)
	{
		_vertices[1].X = static_cast<float>(font_rect.right);
		_vertices[2].X = static_cast<float>(font_rect.right);
		_vertices[3].X = static_cast<float>(font_rect.right);
		_vertices[2].Y = static_cast<float>(font_rect.bottom);
		_vertices[3].Y = static_cast<float>(font_rect.bottom);
		_vertices[4].Y = static_cast<float>(font_rect.bottom);

		void * pVoid;
		_vBuffer->Lock(0, 0, (void**)&pVoid, 0);
		memcpy(pVoid,_vertices,sizeof(_vertices));
		_vBuffer->Unlock();
	}

    _d3dDevice->SetFVF(CUSTOMFVF);
    _d3dDevice->SetStreamSource(0, _vBuffer, 0, sizeof(CUSTOMVERTEX));
    _d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

	_font->DrawText(NULL,        //pSprite
		information.c_str(),  //pString
        -1,          //Count
        &font_rect,  //pRect
        DT_LEFT|DT_NOCLIP,//Format,
        0xFFFFFFFF); //Color
}

void MGDFApp::FatalError(const std::string &message)
{
	_system->FatalError("MGDF",message);
}

void MGDFApp::ExternalClose()
{
	_system->QueueShutDown();
}

/**
shut down the application
*/
void MGDFApp::ShutDownCallBack() {
	_internalShutDown = true;
	PostMessage(MGDFApp::Instance()._window,WM_CLOSE,NULL,NULL);
}

}}