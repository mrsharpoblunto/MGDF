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

GraphicsManager::GraphicsManager(IDirect3D9 *d3d)
{
	_d3d = d3d;
	_currentAdaptorMode = NULL;
	_currentMultiSampleLevel = 0;
	_vsync = true;

	int maxAdaptorModes = _d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT,D3DFMT_X8R8G8B8);
	D3DDISPLAYMODE d3ddm;

	for(int mode = 0; mode < maxAdaptorModes; ++mode)
	{
		if( FAILED( _d3d->EnumAdapterModes(D3DADAPTER_DEFAULT,D3DFMT_X8R8G8B8,mode, &d3ddm )))
		{
			break;
		}

		// Does this adaptor mode support  the desired format and is it above the minimum required resolution
		if( d3ddm.Format == D3DFMT_X8R8G8B8 && d3ddm.Width>=Resources::MIN_SCREEN_X && d3ddm.Height >=Resources::MIN_SCREEN_Y)
		{
			GraphicsAdaptorMode *adaptorMode = new GraphicsAdaptorMode(d3ddm.Width,d3ddm.Height,d3ddm.RefreshRate);
			_adaptorModes.Add(adaptorMode);
		}
	}

	if(FAILED(_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,true,D3DMULTISAMPLE_NONMASKABLE,&_multiSampleLevelWindowed)))
	{
		_multiSampleLevelWindowed = 0;
	}

	if(FAILED(_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,true,D3DMULTISAMPLE_NONMASKABLE,&_multiSampleLevelFullScreen)))
	{
		_multiSampleLevelFullScreen = 0;
	}
}

GraphicsManager::~GraphicsManager(void)
{
	for (std::vector<IGraphicsAdaptorMode *>::const_iterator iter = _adaptorModes.Items()->begin();iter!=_adaptorModes.Items()->end();++iter) {
		delete (GraphicsAdaptorMode *)(*iter);
	}
}

unsigned int GraphicsManager::GetMultiSampleLevels() const
{
	return _multiSampleLevelFullScreen;
}

void GraphicsManager::SetCurrentMultiSampleLevel(unsigned int multisampleLevel)
{
	boost::mutex::scoped_lock lock(_mutex);
	if (multisampleLevel <= _multiSampleLevelFullScreen) {
		_currentMultiSampleLevel = multisampleLevel;
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
			else if ((*iter)->GetRefreshRate()>mode->GetRefreshRate()) {
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

void GraphicsManager::QueueResetDevice()
{
	boost::mutex::scoped_lock lock(_mutex);
	_resetDevicePending = true;
}

bool GraphicsManager::IsResetDevicePending()
{
	return _resetDevicePending;
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
	if (_currentMultiSampleLevel>_multiSampleLevelFullScreen) {
		_currentMultiSampleLevel=_multiSampleLevelFullScreen;
		game->SetPreference(PreferenceConstants::MULTISAMPLE_LEVEL,boost::lexical_cast<std::string>(_currentMultiSampleLevel).c_str());
		savePreferences = true;
	}

	if (savePreferences) {
		game->SavePreferences();
	}
}

void GraphicsManager::OnResetDevice(D3DPRESENT_PARAMETERS *d3dPP,bool toggleFullScreen)
{
	boost::mutex::scoped_lock lock(_mutex);
	if (toggleFullScreen) _fullScreen = !_fullScreen;

	//set up in fullscreen or windowed mode (MGDF will set the res for windowed mode, so don't bother setting it here)
	if (_fullScreen) {
		d3dPP->BackBufferWidth  = _currentAdaptorMode->GetWidth(); 
		d3dPP->BackBufferHeight = _currentAdaptorMode->GetHeight();
		d3dPP->FullScreen_RefreshRateInHz = _currentAdaptorMode->GetRefreshRate();
		d3dPP->BackBufferFormat = D3DFMT_X8R8G8B8;
		d3dPP->Windowed = false;

		//enable multisampling if it was enabled via settings.
		int cmsl = _currentMultiSampleLevel>_multiSampleLevelFullScreen ? _multiSampleLevelFullScreen : _currentMultiSampleLevel;
		if (cmsl>0) {
			d3dPP->MultiSampleType	      = D3DMULTISAMPLE_NONMASKABLE;
			d3dPP->MultiSampleQuality	  =	cmsl - 1;
		}
		else {
			d3dPP->MultiSampleType        = D3DMULTISAMPLE_NONE;
			d3dPP->MultiSampleQuality     = 0;
		}
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

	_resetDevicePending = false;
}

}}