#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

class Module : public MGDF::ComBase<MGDF::IModule> {
 public:
  virtual ~Module(void);
  Module();

  bool STNew(MGDF::ISimHost *host, const wchar_t *workingFolder) final;
  bool STUpdate(MGDF::ISimHost *host, double elapsedTime) final;
  void STShutDown(MGDF::ISimHost *host) final;

  bool RTBeforeFirstDraw(MGDF::IRenderHost *host) final;
  bool RTDraw(MGDF::IRenderHost *host, double alpha) final;
  bool RTBeforeBackBufferChange(MGDF::IRenderHost *host) final;
  bool RTBackBufferChange(MGDF::IRenderHost *host) final;
  bool RTBeforeDeviceReset(MGDF::IRenderHost *host) final;
  bool RTDeviceReset(MGDF::IRenderHost *host) final;

  void Panic() final;

 private:
  bool _inited;
  std::wstring _workingFolder;
};
