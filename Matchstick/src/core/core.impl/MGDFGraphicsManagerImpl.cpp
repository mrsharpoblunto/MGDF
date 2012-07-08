#include "StdAfx.h"

#include <boost/lexical_cast.hpp>

#include "../common/MGDFResources.hpp"
#include "MGDFGraphicsManagerImpl.hpp"
#include "MGDFPreferenceConstants.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

GraphicsManager::GraphicsManager(IDXGIAdapter1 *adapter,ID3D11Device *device)
{
	IDXGIOutput *output;
	if (FAILED(adapter->EnumOutputs(0, &output)))
	{
		return;
	}

	_device = device;
	_currentAdaptorMode = NULL;
	_currentMultiSampleLevel = 1;
	_vsync = true;

	UINT maxAdaptorModes;
	if (FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_ENUM_MODES_INTERLACED,&maxAdaptorModes,NULL)))
	{
		return;
	}

	DXGI_MODE_DESC *modes = new DXGI_MODE_DESC[maxAdaptorModes];
	if (FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_ENUM_MODES_INTERLACED,&maxAdaptorModes,modes)))
	{
		delete[] modes;
		return;
	}

	for(int mode = 0; mode < maxAdaptorModes; ++mode)
	{
		DXGI_MODE_DESC *displayMode	= &modes[mode];

		// Does this adaptor mode support  the desired format and is it above the minimum required resolution
		if( displayMode->Format == DXGI_FORMAT_R8G8B8A8_UNORM && displayMode->Width>=Resources::MIN_SCREEN_X && displayMode->Height >=Resources::MIN_SCREEN_Y)
		{
			GraphicsAdaptorMode *adaptorMode = new GraphicsAdaptorMode(displayMode->Width,displayMode->Height,displayMode->RefreshRate.Numerator,displayMode->RefreshRate.Denominator);
			_adaptorModes.Add(adaptorMode);
		}
	}

	delete[] modes;

	//determine the supported multisampling settings for this device
	for (int i=1;i<D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;++i)
	{
		unsigned int quality=0;
		if (FAILED(_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality)) || quality==0)
		{
			continue;
		}

		_multiSampleLevels.Add(i);
		_multiSampleQuality[i] = quality;
	}
}

GraphicsManager::~GraphicsManager(void)
{
	for (std::vector<IGraphicsAdaptorMode *>::const_iterator iter = _adaptorModes.Items()->begin();iter!=_adaptorModes.Items()->end();++iter) {
		delete (GraphicsAdaptorMode *)(*iter);
	}
}

IUIntList *GraphicsManager::GetMultiSampleLevels() const
{
	return (IUIntList *)&_multiSampleLevels;
}

bool GraphicsManager::SetCurrentMultiSampleLevel(unsigned int multisampleLevel)
{
	boost::mutex::scoped_lock lock(_mutex);
	if (_multiSampleQuality.find(multisampleLevel)!=_multiSampleQuality.end()) 
	{
		_currentMultiSampleLevel = multisampleLevel;
		return true;
	}
	else
	{
		return false;
	}
}

unsigned int GraphicsManager::GetCurrentMultiSampleLevel() const
{
	return _currentMultiSampleLevel;
}

bool GraphicsManager::GetVSync() const
{
	return _vsync;
}

void GraphicsManager::SetVSync(bool vsync)
{
	boost::mutex::scoped_lock lock(_mutex);
	_vsync = vsync;
}

const IGraphicsAdaptorModeList *GraphicsManager::GetAdaptorModes() const
{
	return &_adaptorModes;
}

IGraphicsAdaptorMode *GraphicsManager::GetAdaptorMode(unsigned int width,unsigned int height) const
{
	IGraphicsAdaptorMode *mode = NULL;
	for (std::vector<IGraphicsAdaptorMode *>::const_iterator iter = _adaptorModes.Items()->begin();iter!=_adaptorModes.Items()->end();++iter) {
		if ((*iter)->GetWidth()==width && (*iter)->GetHeight()==height) {
			if (mode==NULL) {
				mode = (*iter);
			}
			else if (
				(*iter)->GetRefreshRateNumerator()>=mode->GetRefreshRateNumerator() && 
				(*iter)->GetRefreshRateDenominator()<=mode->GetRefreshRateDenominator()) {
				mode = (*iter);
			}
		}
	}
	return mode;
}

IGraphicsAdaptorMode *GraphicsManager::GetCurrentAdaptorMode() const
{
	return _currentAdaptorMode;
}

unsigned int GraphicsManager::GetScreenX() const
{
	return _currentAdaptorMode->GetWidth();
}

unsigned int GraphicsManager::GetScreenY() const
{
	return _currentAdaptorMode->GetHeight();
}

void GraphicsManager::SetCurrentAdaptorMode(IGraphicsAdaptorMode *mode)
{
	boost::mutex::scoped_lock lock(_mutex);
	_currentAdaptorMode = mode;
}

void GraphicsManager::ApplyChanges()
{
	boost::mutex::scoped_lock lock(_mutex);
	_changePending = true;
}

bool GraphicsManager::IsChangePending()
{
	return _changePending;
}

void GraphicsManager::SetBackBuffer(ID3D11Texture2D *backBuffer)
{
	_backBuffer = backBuffer;
}

ID3D11Texture2D *GraphicsManager::GetBackBuffer()
{
	return _backBuffer;
}

ID3D11Device *GraphicsManager::GetD3DDevice()
{
	return _device;
}

void GraphicsManager::LoadPreferences(IGame *game) 
{
	bool savePreferences = false;
	_fullScreen = strcmp("1",game->GetPreference(PreferenceConstants::FULL_SCREEN))==0;
	_vsync = strcmp("1",game->GetPreference(PreferenceConstants::VSYNC))==0;

	_currentAdaptorMode=GetAdaptorMode(
		atoi(game->GetPreference(PreferenceConstants::SCREEN_X)),
		atoi(game->GetPreference(PreferenceConstants::SCREEN_Y)));
	if (_currentAdaptorMode==NULL) {
		//set 1024*768 as the default
		_currentAdaptorMode = GetAdaptorMode(Resources::MIN_SCREEN_X,Resources::MIN_SCREEN_Y);
		//or if 1024*768 was unavailble the first adaptor mode in the list
		if (_currentAdaptorMode == NULL) {
			_currentAdaptorMode = _adaptorModes.Get(0);
		}
		//try to find the widescreen resolution if possible, otherwise stick to the default found above if none are found.
		for (std::vector<IGraphicsAdaptorMode *>::const_iterator iter = _adaptorModes.Items()->begin();iter!=_adaptorModes.Items()->end();++iter) {
			float apsectRatio = (*iter)->GetWidth()/(float)(*iter)->GetHeight();
			if ((*iter)->GetWidth()>_currentAdaptorMode->GetWidth() && (*iter)->GetHeight()>_currentAdaptorMode->GetHeight() && apsectRatio>1.34) {
				_currentAdaptorMode = (*iter);
				break;
			}
		}
		game->SetPreference(PreferenceConstants::FULL_SCREEN,(boost::lexical_cast<std::string>(_currentAdaptorMode->GetWidth())+"*"+boost::lexical_cast<std::string>(_currentAdaptorMode->GetHeight())).c_str());
		game->SetPreference(PreferenceConstants::SCREEN_X,boost::lexical_cast<std::string>(_currentAdaptorMode->GetWidth()).c_str());
		game->SetPreference(PreferenceConstants::SCREEN_Y,boost::lexical_cast<std::string>(_currentAdaptorMode->GetHeight()).c_str());
		savePreferences = true;
	}

	_currentMultiSampleLevel = atoi(game->GetPreference(PreferenceConstants::MULTISAMPLE_LEVEL));
	if (_currentMultiSampleLevel>_multiSampleLevels.Get(_multiSampleLevels.Size())) {
		_currentMultiSampleLevel=_multiSampleLevels.Get(_multiSampleLevels.Size());
		game->SetPreference(PreferenceConstants::MULTISAMPLE_LEVEL,boost::lexical_cast<std::string>(_currentMultiSampleLevel).c_str());
		savePreferences = true;
	}

	if (savePreferences) {
		game->SavePreferences();
	}
}

void GraphicsManager::OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *desc)
{
	boost::mutex::scoped_lock lock(_mutex);
	if (toggleFullScreen) _fullScreen = !_fullScreen;

	//set up in fullscreen or windowed mode (MGDF will set the res for windowed mode, so don't bother setting it here)
	if (_fullScreen) {
		desc->BufferDesc.Width  = _currentAdaptorMode->GetWidth(); 
		desc->BufferDesc.Height = _currentAdaptorMode->GetHeight();
		desc->BufferDesc.RefreshRate.Numerator = _currentAdaptorMode->GetRefreshRateNumerator();
		desc->BufferDesc.RefreshRate.Denominator = _currentAdaptorMode->GetRefreshRateDenominator();

		//desc->BufferDesc.Format = //TODO;
		desc->Windowed = false;

		desc->SampleDesc.Count	      = _currentMultiSampleLevel;
		desc->SampleDesc.Quality	  =	_multiSampleQuality[_currentMultiSampleLevel] - 1;
	}
	else {
		d3dPP->BackBufferFormat = D3DFMT_UNKNOWN;
		d3dPP->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		d3dPP->Windowed = true;

		//enable multisampling if it was enabled via settings.
		int cmsl = _currentMultiSampleLevel>_multiSampleLevelWindowed ? _multiSampleLevelWindowed : _currentMultiSampleLevel;
		if (cmsl>0) {
			d3dPP->MultiSampleType	      = D3DMULTISAMPLE_NONMASKABLE;
			d3dPP->MultiSampleQuality	  =	cmsl - 1;
		}
		else {
			d3dPP->MultiSampleType        = D3DMULTISAMPLE_NONE;
			d3dPP->MultiSampleQuality     = 0;
		}
	}

	d3dPP->BackBufferCount            = 1;
	d3dPP->SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
	d3dPP->EnableAutoDepthStencil     = true; 
	d3dPP->AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dPP->Flags                      = 0;
	d3dPP->PresentationInterval       = _vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

	_pendingChanges = false;
}

}}