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

  bool STNew(MGDF::ISimHost *host, const wchar_t *workingFolder) override final;
  bool STUpdate(MGDF::ISimHost *host, double elapsedTime) override final;
  void STShutDown(MGDF::ISimHost *host) override final;

  bool RTBeforeFirstDraw(MGDF::IRenderHost *host) override final;
  bool RTDraw(MGDF::IRenderHost *host, double alpha) override final;
  bool RTBeforeBackBufferChange(MGDF::IRenderHost *host) override final;
  bool RTBackBufferChange(MGDF::IRenderHost *host) override final;
  bool RTBeforeDeviceReset(MGDF::IRenderHost *host) override final;
  bool RTDeviceReset(MGDF::IRenderHost *host) override final;

  void Panic() override final;

 private:
  bool _inited;
  std::wstring _workingFolder;
};
