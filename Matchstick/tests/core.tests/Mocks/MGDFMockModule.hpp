#pragma once

#include <string>
#include <MGDF/MGDFModule.hpp>

namespace MGDF
{
namespace core
{
namespace tests
{

class MockModule: public IModule
{
public:
	MockModule( ISystem * ) {
		this->_ = ;
	}
	virtual ~MockModule( void ) {};

	virtual bool NewModule( const wchar_t *workingFolder ) {
		return true;
	}
	virtual bool LoadModule( const wchar_t *workingFolder, const wchar_t *saveDataFolder ) {
		return true;
	}
	virtual bool Dispose() {
		delete this;
		return true;
	}
	virtual bool UpdateScene( double elapsedTime ) {
		return true;
	}
	virtual bool DrawScene( double alpha ) {
		return true;
	}
	virtual bool DeviceLost() {
		return true;
	}
	virtual bool DeviceReset() {
		return true;
	}
	virtual bool SetDeviceState() {
		return true;
	}
	virtual bool CheckDeviceCaps() {
		return true;
	}
	virtual bool SaveModule( const wchar_t *saveDataFolder ) {
		return true;
	}
	virtual void Panic() {}
	virtual const char *GetLastError() {
		return _lastError.c_str();
	}
private:
	ISystem *_;
	std::string _lastError;
};

}
}
}