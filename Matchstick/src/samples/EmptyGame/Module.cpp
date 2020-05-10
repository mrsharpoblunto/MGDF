#include "StdAfx.h"

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

Module::~Module(void) {}
Module::Module() {}

bool Module::STNew(MGDF::ISimHost *host, const wchar_t *workingFolder) {
  // This method is called by the host to initialize the module.
  // the workingFolder is a folder on disk that the game can use
  // to write out temporary files if needed.
  return true;
}
bool Module::STUpdate(MGDF::ISimHost *host, double elapsedTime) {
  // Game logic goes here, the host calls this function once per
  // tick of the simulation thread
  return true;
}
void Module::STShutDown(MGDF::ISimHost *host) {
  // Called by the host to indicate to the module that it should shut down
  // as soon as possible. Note that shutdown will not actually occur until
  // the module then calls host->Shutdown(), which it can do at its discretion
  host->ShutDown();
}
bool Module::STDispose(MGDF::ISimHost *host) {
  // called by the host when the module is to be destroyed
  delete this;
  return true;
}
bool Module::RTBeforeFirstDraw(MGDF::IRenderHost *host) {
  // Called by the host before any rendering occurs on the render thread
  // Any first time rendering initialization stuff should go here
  return true;
}
bool Module::RTDraw(MGDF::IRenderHost *host, double alpha) {
  // Called by the host once per tick of the render thread
  // Any rendering goes here.
  return true;
}
bool Module::RTBeforeBackBufferChange(MGDF::IRenderHost *host) {
  // Called before the host resizes the current backbuffer
  // Anything holding a reference to the backbuffer should release it now
  // otherwise Direct3D will not be able to change the backbuffer
  return true;
}
bool Module::RTBackBufferChange(MGDF::IRenderHost *host) {
  // Called after the host has created a newly resized back buffer
  // Any Direct3D resources that may need to be updated as a result of the
  // backbuffer changing should be handled here.
  return true;
}
bool Module::RTBeforeDeviceReset(MGDF::IRenderHost *host) {
  // Called by the host before resetting the Direct3D Device
  // Anything holding a reference to ANY device dependent resource should
  // release it now as these references to the removed device are now invalid
  return true;
}
bool Module::RTDeviceReset(MGDF::IRenderHost *host) {
  // Called by the host after resetting the Direct3D device
  // Recreate any resources cleared out in RTBeforeDeviceReset
  return true;
}
void Module::Panic() {
  // Called by the host after any module event fails
}
