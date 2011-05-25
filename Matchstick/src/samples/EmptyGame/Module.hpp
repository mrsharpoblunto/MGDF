#pragma once

#include <time.h>
#include "d3d9.h"
#include <MGDF/MGDF.hpp>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

class Module: public MGDF::IModule 
{
public:
	virtual ~Module(void);
	Module(MGDF::ISystem *);

	virtual bool NewModule(const char *workingFolder);
	virtual bool LoadModule(const char *,const char *);//file to load from
	virtual bool SaveModule(const char *);//file to save to

	virtual bool Dispose(void);

	virtual bool DrawScene(double alpha);
	virtual bool DeviceLost();
	virtual bool DeviceReset();
	virtual bool SetDeviceState();
	virtual bool CheckDeviceCaps();

	virtual bool UpdateScene(double elapsedTime);
	virtual void Panic();

	const char *GetLastError();
private:
	MGDF::ISystem *_system;
	bool _inited;
	std::string _lastError,_workingFolder;
};

