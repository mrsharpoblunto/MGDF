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

ISystemEvent *EventFactory::CreateLoadEvent(std::string loadFile) {
	return new LoadEvent(loadFile);
}

ISystemEvent *EventFactory::CreateNewEvent() {
	return new NewEvent();
}

}}}
