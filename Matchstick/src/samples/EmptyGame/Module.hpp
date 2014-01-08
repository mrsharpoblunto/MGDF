#pragma once

#include <MGDF/MGDF.hpp>


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

class Module: public MGDF::IModule
{
public:
	virtual ~Module( void );
	Module();

	bool STNew( MGDF::ISimHost *host, const wchar_t *workingFolder ) override;
	bool STUpdate( MGDF::ISimHost *host, double elapsedTime ) override;
	void STShutDown( MGDF::ISimHost *host ) override;
	bool STDispose( MGDF::ISimHost *host ) override;

	bool RTBeforeFirstDraw( MGDF::IRenderHost *host ) override;
	bool RTDraw( MGDF::IRenderHost *host, double alpha ) override;
	bool RTBeforeBackBufferChange( MGDF::IRenderHost *host ) override;
	bool RTBackBufferChange( MGDF::IRenderHost *host ) override;
	bool RTBeforeDeviceReset( MGDF::IRenderHost *host ) override;
	bool RTDeviceReset( MGDF::IRenderHost *host ) override;

	void Panic() override;
private:
	bool _inited;
	std::wstring _workingFolder;
};

