#pragma once

#include <MGDF/MGDF.hpp>


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

class Module: public MGDF::IModule 
{
public:
	virtual ~Module(void);
	Module(MGDF::ISystem *);

	virtual bool New(const wchar_t *workingFolder);

	virtual bool Dispose(void);

	virtual bool DrawScene(double alpha);
	virtual bool BackBufferChanged();
	virtual bool UpdateScene(double elapsedTime);
	virtual void Panic();
	virtual void ShutDown();

	const char *GetLastError();
private:
	MGDF::ISystem *_system;
	bool _inited;
	std::string _lastError;
	std::wstring _workingFolder;
};

