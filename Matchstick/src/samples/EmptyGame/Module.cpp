#include "StdAfx.h"

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

Module::~Module(void) {}
Module::Module() : _inited(false) {}

BOOL Module::STNew(IMGDFSimHost *host) {
  // This method is called by the host to initialize the module.
  return true;
}
BOOL Module::STUpdate(IMGDFSimHost *host, double elapsedTime) {
  // Game logic goes here, the host calls this function once per
  // tick of the simulation thread
  return true;
}
void Module::STShutDown(IMGDFSimHost *host) {
  // Called by the host to indicate to the module that it should shut down
  // as soon as possible. Note that shutdown will not actually occur until
  // the module then calls host->Shutdown(), which it can do at its discretion
  host->ShutDown();
}
BOOL Module::RTBeforeFirstDraw(IMGDFRenderHost *host) {
  // Called by the host before any rendering occurs on the render thread
  // Any first time rendering initialization stuff should go here
  return true;
}
BOOL Module::RTDraw(IMGDFRenderHost *host, double alpha) {
  // Called by the host once per tick of the render thread
  // Any rendering goes here.
  return true;
}
BOOL Module::RTBeforeBackBufferChange(IMGDFRenderHost *host) {
  // Called before the host resizes the current backbuffer
  // Anything holding a reference to the backbuffer should release it now
  // otherwise Direct3D will not be able to change the backbuffer
  return true;
}
BOOL Module::RTBackBufferChange(IMGDFRenderHost *host) {
  // Called after the host has created a newly resized back buffer
  // Any Direct3D resources that may need to be updated as a result of the
  // backbuffer changing should be handled here.
  return true;
}
BOOL Module::RTBeforeDeviceReset(IMGDFRenderHost *host) {
  // Called by the host before resetting the Direct3D Device
  // Anything holding a reference to ANY device dependent resource should
  // release it now as these references to the removed device are now invalid
  return true;
}
BOOL Module::RTDeviceReset(IMGDFRenderHost *host) {
  // Called by the host after resetting the Direct3D device
  // Recreate any resources cleared out in RTBeforeDeviceReset
  return true;
}
void Module::Panic() {
  // Called by the host after any module event fails
}
