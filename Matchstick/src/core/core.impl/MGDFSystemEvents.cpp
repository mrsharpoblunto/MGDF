#include "StdAfx.h"
#include "MGDFSystemEvents.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace events {

ISystemEvent *EventFactory::CreateSaveEvent(std::string saveFile) {
	return new SaveEvent(saveFile);
}

ISystemEvent *EventFactory::CreateLoadEvent(std::string loadFile,bool isBootLoad) {
	return new LoadEvent(loadFile,isBootLoad);
}

ISystemEvent *EventFactory::CreatePushLoadEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser,std::string loadDataDir) {
	return new PushLoadEvent(moduleName,moduleInitialiser,loadDataDir);
}

ISystemEvent *EventFactory::CreatePushNewEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser) {
	return new PushNewEvent(moduleName,moduleInitialiser);
}

ISystemEvent *EventFactory::CreateSwapEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser) {
	return new SwapEvent(moduleName,moduleInitialiser);
}

ISystemEvent *EventFactory::CreatePopEvent(unsigned int popCount) {
	return new PopEvent(popCount);
}

}}}
