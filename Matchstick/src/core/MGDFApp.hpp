#pragma once

#include "MGDFD3DAppFramework.hpp"
#include "core.impl/MGDFSystemImpl.hpp"

namespace MGDF { namespace core {

#include "common/MGDFSingleton.hpp"

struct CUSTOMVERTEX {FLOAT X, Y, Z, RHW; DWORD COLOR;};
#define CUSTOMFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

class MGDFApp: public D3DAppFramework, public Singleton<MGDFApp>
{
public:
	DECLARE_SINGLETON(MGDFApp);

	MGDFApp(HINSTANCE hInstance);
	~MGDFApp();
	virtual bool CheckDeviceCaps();
	virtual void OnInitD3D(ID3D11Device *device,IDXGIAdapter1 *adapter);
	virtual void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *,bool toggleFullScreen);
	virtual bool IsResetSwapChainPending();	
	virtual void UpdateScene(double elapsedTime);
	virtual void DrawScene(double alpha);
	virtual void OnLostDevice();
	virtual void OnResetDevice();
	virtual void FatalError(const std::string &message);
	virtual void ExternalClose();
	virtual void InitDirect3D(const std::string &caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen = true);

	void SetSystem(System *system);
private:
	void ShutDownCallBack();

	void DrawSystemOverlay();

	bool _initialized;
	double _alpha;

	System *_system;
	IDXGISwapChain* _swapChain;
	ID3DXFont *_font;
	CUSTOMVERTEX _vertices[6];
	LPDIRECT3DVERTEXBUFFER9 _vBuffer;
};

}}