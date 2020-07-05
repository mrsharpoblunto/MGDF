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

  BOOL __stdcall STNew(IMGDFSimHost *host) final;
  BOOL __stdcall STUpdate(IMGDFSimHost *host, double elapsedTime) final;
  void __stdcall STShutDown(IMGDFSimHost *host) final;

  BOOL __stdcall RTBeforeFirstDraw(IMGDFRenderHost *host) final;
  BOOL __stdcall RTDraw(IMGDFRenderHost *host, double alpha) final;
  BOOL __stdcall RTBeforeBackBufferChange(IMGDFRenderHost *host) final;
  BOOL __stdcall RTBackBufferChange(IMGDFRenderHost *host) final;
  BOOL __stdcall RTBeforeDeviceReset(IMGDFRenderHost *host) final;
  BOOL __stdcall RTDeviceReset(IMGDFRenderHost *host) final;

  void __stdcall Panic() final;

 private:
  bool _inited;
  std::wstring _workingFolder;
};
