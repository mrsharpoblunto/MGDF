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
	LoadEvent(std::string loadname) {
		Loadname = loadname;
	}
	virtual ~LoadEvent(){}

	std::string Loadname;
};

class NewEvent:public ISystemEvent
{
public:
	NewEvent() {
	}
	virtual ~NewEvent(){
	}
};

class EventFactory {
public:
	static ISystemEvent *CreateSaveEvent(std::string saveFile);
	static ISystemEvent *CreateLoadEvent(std::string loadFile);
	static ISystemEvent *CreateNewEvent();
};

}}}