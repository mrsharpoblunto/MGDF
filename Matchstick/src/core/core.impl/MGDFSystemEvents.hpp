#pragma once

#include <string>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFSystem.hpp>

namespace MGDF { namespace core { namespace events {

class ISystemEvent {
public:
	virtual ~ISystemEvent(){}
};

class SaveEvent:public ISystemEvent
{
public:
	SaveEvent(std::string savename) { 
		Savename = savename;
	}
	virtual ~SaveEvent(){}

	std::string Savename;
};

class LoadEvent:public ISystemEvent
{
public:
	LoadEvent(std::string loadname,bool isBootLoad = false) {
		Loadname = loadname;
		IsBootLoad = isBootLoad;
	}
	virtual ~LoadEvent(){}

	bool IsBootLoad;
	std::string Loadname;
};

class PushNewEvent:public ISystemEvent
{
public:
	PushNewEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser) {
		ModuleName = moduleName;
		ModuleInitialiser = moduleInitialiser;
	}
	virtual ~PushNewEvent(){
		SAFE_DELETE(ModuleInitialiser);
	}

	std::string ModuleName;
	IModuleInitialiser *ModuleInitialiser;

};

class PushLoadEvent:public PushNewEvent
{
public:
	PushLoadEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser,std::string loadDataDir) 
	:PushNewEvent(moduleName,moduleInitialiser)
	{
		LoadDataDir = loadDataDir;
	}

	virtual ~PushLoadEvent(){}

	std::string LoadDataDir;
};

class SwapEvent: public PushNewEvent
{
public:
	SwapEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser) 
	:PushNewEvent(moduleName,moduleInitialiser){}
	virtual ~SwapEvent(){}
};

class PopEvent: public ISystemEvent
{
public:
	PopEvent(unsigned int popCount) {
		PopCount = popCount;
	}
	virtual ~PopEvent(){}

	unsigned int PopCount;
};

class EventFactory {
public:
	static ISystemEvent *CreateSaveEvent(std::string saveFile);
	static ISystemEvent *CreateLoadEvent(std::string loadFile,bool isBootLoad);
	static ISystemEvent *CreatePushNewEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser);
	static ISystemEvent *CreatePushLoadEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser,std::string loadDataDir);
	static ISystemEvent *CreateSwapEvent(std::string moduleName,IModuleInitialiser *moduleInitialiser);
	static ISystemEvent *CreatePopEvent(unsigned int popCount);
};

}}}