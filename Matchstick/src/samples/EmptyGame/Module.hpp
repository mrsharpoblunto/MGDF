#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

class Module : public MGDF::ComBase<IMGDFModule> {
 public:
  virtual ~Module(void);
  Module();

  BOOL STNew(IMGDFSimHost *host) final;
  BOOL STUpdate(IMGDFSimHost *host, double elapsedTime) final;
  void __stdcall STShutDown(IMGDFSimHost *host) final;

  BOOL RTBeforeFirstDraw(IMGDFRenderHost *host) final;
  BOOL RTDraw(IMGDFRenderHost *host, double alpha) final;
  BOOL RTBeforeBackBufferChange(IMGDFRenderHost *host) final;
  BOOL RTBackBufferChange(IMGDFRenderHost *host) final;
  BOOL RTBeforeDeviceReset(IMGDFRenderHost *host) final;
  BOOL RTDeviceReset(IMGDFRenderHost *host) final;

  void __stdcall Panic() final;

 private:
  bool _inited;
  std::wstring _workingFolder;
};
