#pragma once

#include "FW1FontWrapper/FW1FontWrapper.h"

#include "MGDFD3DAppFramework.hpp"
#include "core.impl/MGDFSystemImpl.hpp"

namespace MGDF { namespace core {

#include "common/MGDFSingleton.hpp"

//struct CUSTOMVERTEX {FLOAT X, Y, Z, RHW; DWORD COLOR;};
//#define CUSTOMFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

class MGDFApp: public D3DAppFramework, public Singleton<MGDFApp>
{
public:
	DECLARE_SINGLETON(MGDFApp);

	MGDFApp(HINSTANCE hInstance);
	virtual ~MGDFApp();
	
	virtual void OnInitD3D(ID3D11Device *device, IDXGIAdapter1 *adapter);
	virtual void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *,BOOL *);
	virtual bool IsBackBufferChangePending();	
	virtual void OnBackBufferChanged(ID3D11Texture2D *backBuffer);
	virtual void UpdateScene(double elapsedTime);
	virtual void DrawScene(double alpha);
	virtual void FatalError(const std::string &message);
	virtual void ExternalClose();
	virtual void InitDirect3D(const std::string &caption,WNDPROC windowProcedure);

	void SetSystem(System *system);
private:
	void DrawSystemOverlay();

	bool _initialized;
	double _alpha;
	System *_system;
	IFW1FontWrapper *_font;
};

}}