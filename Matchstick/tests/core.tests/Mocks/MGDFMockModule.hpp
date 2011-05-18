#pragma once

#include <string>
#include <MGDF/MGDFModule.hpp>

namespace MGDF { namespace core { namespace tests {

class MockModule: public IModule
{
public:
	MockModule(std::string name,ISystem *system) {
		this->_name = name;
		this->_system = system;
	}
	virtual ~MockModule(void){};

	std::string GetName() { return _name; }

	virtual bool NewModule(const char *workingFolder) { return true; }
	virtual bool LoadModule(const char *workingFolder,const char *saveDataFolder) { return true; }
	virtual bool Dispose() { delete this; return true; }
	virtual bool UpdateScene(double elapsedTime) { return true; }
	virtual bool DrawScene(double alpha) { return true; }
	virtual bool DeviceLost() { return true; }
	virtual bool DeviceReset() { return true; }
	virtual bool SetDeviceState() { return true; }
	virtual bool CheckDeviceCaps() { return true; }
	virtual bool SaveModule(const char *saveDataFolder){ return true; }
	virtual void Panic(){}
	virtual const char *GetLastError() { return _lastError.c_str(); }
private:
	std::string _name;
	ISystem *_system;
	std::string _lastError;
};

}}}