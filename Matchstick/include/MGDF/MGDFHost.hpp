#pragma once

#include <d3d11.h>

#include <MGDF/MGDFDebug.hpp>
#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFGame.hpp>
#include <MGDF/MGDFInputManager.hpp>
#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFModule.hpp>
#include <MGDF/MGDFRenderSettingsManager.hpp>
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFStatisticsManager.hpp>
#include <MGDF/MGDFString.hpp>
#include <MGDF/MGDFTimer.hpp>
#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF {

DECLARE_LIST(IStringList, const char *)

/**
 Provides an entrypoint for a module to interact with the MGDF host. Methods in
 this interface are safe to be used from any thread
*/
MIDL_INTERFACE("B6FDBA44-D098-4EF7-AF79-7368FB85AA6C")
ICommonHost : public ILogger {
 public:
  /**
   * This method should be invoked when a fatal error is encountered
   * \param sender the location or component the error occurred in
   * \param message details of the fatal error
   */
  virtual void FatalError(const char *sender, const char *message) = 0;

  /**
  get the render settings manager
  \return the render settings manager
  */
  virtual void GetRenderSettings(IRenderSettingsManager * *settings) = 0;

  /**
  get the host timer
  \param timer pointer to the host timer
  */
  virtual void GetTimer(ITimer * *timer) = 0;

  /**
  gets the current version of the framework
  */
  virtual const Version *GetMGDFVersion() const = 0;

  /**
  get the virtual filesystem
  \return the virtual filesystem
  */
  virtual void GetVFS(IVirtualFileSystem * *vfs) = 0;

  /**
  get the direct3d device object from the host
  \return the direct3d device object from the host
  */
  virtual ID3D11Device *GetD3DDevice() const = 0;

  /**
  get the direct2d device object from the host
  \return the direct2d device object from the host
  */
  virtual ID2D1Device *GetD2DDevice() const = 0;

  /**
  get a text description for an MGDF error code
  \param err the error code to get a description for
  \return a text description for an MGDF error code
  */
  virtual const char *GetErrorDescription(MGDFError err) const = 0;

  /**
  get a text name for an MGDF error code
  \param err the error code to get the name of
  \return a text name for an MGDF error code
  */
  virtual const char *GetErrorString(MGDFError err) const = 0;

  /**
  get the onscreen debug overlay interface
  \param debug pointer to a IDebug
  */
  virtual void GetDebug(IDebug * *debug) = 0;

  /**
   create a performance counter for profiling CPU time taken. When no longer
   used it should be Released
  \param name the name of the counter \param counter
   points to the created counter \return MGDF_OK if the counter could be
   created, otherwise an error code is returned
   */
  virtual HRESULT CreateCPUCounter(const char *name,
                                   IPerformanceCounter **counter) = 0;
};

/**
 Provides an entrypoint for a module to interact with the MGDF host. This
 interface inherits from ICommonHost and provides additional methods which are
 safe to be used ONLY from the render thread.
*/
MIDL_INTERFACE("514EBD97-3C53-4235-90E1-7AD3B7F517BD")
IRenderHost : public ICommonHost {
 public:
  /**
  get the direct3d device immediate context object from the host
  \return the direct3d deviceimmediate context object from the host
  */
  virtual ID3D11DeviceContext *GetD3DImmediateContext() const = 0;

  /**
  set the current back buffer as the render target for the specified d2d device
  context \param context the d2d device context to set the render target for
  \return true if the back buffer can be set as the render target for the device
  context
  */
  virtual bool SetBackBufferRenderTarget(ID2D1DeviceContext * context) = 0;

  /**
  Gets the current back buffer texture. The pointer returned by this method
  becomes invalid when the modules OnReset event is fired
  */
  virtual ID3D11Texture2D *GetBackBuffer() const = 0;

  /**
  Gets the current depth stencil buffer texture. The pointer returned by this
  method becomes invalid when the modules OnReset event is fired
  */
  virtual ID3D11Texture2D *GetDepthStencilBuffer() const = 0;

  /**
  Gets the description of the backbuffer and depthStencilBuffer textures.
  \param backBufferDesc the backbuffer description to populate. if null is
  passed, this field is not populated \param depthStencilDesc the backbuffer
  description to populate. if null is passed, this field is not populated
  */
  virtual void GetBackBufferDescription(
      D3D11_TEXTURE2D_DESC * backBufferDesc,
      D3D11_TEXTURE2D_DESC * depthStencilBufferDesc) const = 0;

  /**
   * create a performance counter for profiling GPU time taken in DirectX API
   * calls. When no longer used it should be Released \param name the name of
   * the counter \param counter points to the created counter \return
   * MGDF_ERR_GPU_TIMER_UNSUPPORTED if GPU counters are unsupported (using
   * D3D_FEATURE_LEVEL_9_3 or less) otherwise returns MGDF_OK
   */
  virtual HRESULT CreateGPUCounter(const char *name,
                                   IPerformanceCounter **counter) = 0;
};

/**
Provides an entrypoint for a module to interact with the MGDF host. This
interface inherits from ICommonHost and provides additional methods which are
safe to be used ONLY from the sim thread.
*/
MIDL_INTERFACE("381AD5F1-8058-4739-992F-A5B551AA0E87")
ISimHost : public ICommonHost {
 public:
  /**
   tells the host to provide a location on disk to save the current game data.
   After saving the data it is required that CompleteSave is called using the
   same saveName parameter. \param saveName the name of the module save file.
   Only alphanumeric characters and space are valid characters. \param
   saveBuffer the buffer to fill in the supplied save directory \param size the
   size of saveBuffer, if saveBuffer is too small, size will be changed to the
   size required. \return MGDF_OK if saveBuffer is large enough to fit the
   supplied save directory, otherwise returns MGDF_ERR_BUFFER_TOO_SMALL. If the
   saveName is invalid, the function returns an error code
  */
  virtual MGDFError BeginSave(const char *saveName, wchar_t *saveBuffer,
                              UINT32 *size) = 0;

  /**
   finalizes the save data for a matching call to BeginSave
   \param saveName the save to complete
   \return MGDF_OK if the saveName was in a pending state and was completed
   successfully, if there was a problem, or the saveName didn't exist then an
   error code is returned.
   */
  virtual MGDFError CompleteSave(const char *saveName) = 0;

  /**
   populates the supplied vector with the names of all saved instances of this
   configuration The names returned in this list represent all the valid
   arguments to queueLoadState for the current configuration \return the list to
   fill with save names
  */
  virtual const IStringList *GetSaves() const = 0;

  /**
   deletes a selected save game from the hard drive
   \param saveName the save to remove
   */
  virtual void RemoveSave(const char *saveName) = 0;

  /**
   tells the host to find the location on disk for the specified save game
   \param saveName the name of the module save file
   \param loadBuffer the buffer to fill in the supplied save directory
   \param size the size of saveBuffer, if saveBuffer is too small, size will be
   changed to the size required. \param version the version number of the save
   game. can be useful for migrating save games. \return MGDF_OK if saveBuffer
   is large enough to fit the supplied load directory, otherwise returns
   MGDF_ERR_BUFFER_TOO_SMALL. If the saveName is invalid, the function returns
   an error code
  */
  virtual MGDFError Load(const char *saveName, wchar_t *loadBuffer,
                         UINT32 *size, Version *version) = 0;

  /**
  get information regarding the current game and its preferences
  \param game pointer to object containing information regarding the game and its preferences
  */
  virtual void GetGame(IGame * *game) = 0;

  /**
  get the audio manager
  \param manager the audio manager, nullptr if the audio subsystem failed to
  initialize
  */
  virtual void GetSound(ISoundManager * *manager) = 0;

  /**
  get the statistics manager
  \return the statistics manager
  */
  virtual void GetStatistics(IStatisticsManager * *statistics) = 0;

  /**
  get the input manager
  \param manager pointer to an input manager
  */
  virtual void GetInput(IInputManager * *manager) = 0;

  /**
  tells the host to shut down the game immediately
  */
  virtual void ShutDown(void) = 0;

  /**
  tells the host to invoke the modules shutdown callback
  */
  virtual void QueueShutDown(void) = 0;
};

}  // namespace MGDF