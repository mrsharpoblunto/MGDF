#pragma once

#include <d3d11.h>
#include <vector>
#include <list>
#include <mutex>
#include <MGDF/MGDFTimer.hpp>

namespace MGDF
{
namespace core
{

class Timer;

class CPUPerformanceCounter: public IPerformanceCounter
{
public:
	virtual ~CPUPerformanceCounter();
	CPUPerformanceCounter( const char *name, Timer *timer );

	const char *GetName() const override final;
	void Begin() override final;
	void End() override final;

	HRESULT QueryInterface( REFIID riid, void **ppvObject ) override final;
	ULONG AddRef() override final;
	ULONG Release() override final;

	double GetAvgValue();
private:
	ULONG _references;
	std::mutex _mutex;
	std::string _name;
	LARGE_INTEGER _start;
	std::list<double> _samples;
	double _avg;
	Timer *_timer;

};

class GPUPerformanceCounter: public IPerformanceCounter
{
public:
	virtual ~GPUPerformanceCounter();
	GPUPerformanceCounter( const char *name, Timer *timer );

	const char *GetName() const override final;
	void Begin() override final;
	void End() override final;

	HRESULT QueryInterface( REFIID riid, void **ppvObject ) override final;
	ULONG AddRef() override final;
	ULONG Release() override final;

	void Reinit();
	double GetAvgValue();
	void SetSample( UINT32 previousFrame, UINT64 frequency );
private:
	ULONG _references;
	std::string _name;
	std::vector<ID3D11Query *> _beginQueries;
	std::vector<ID3D11Query *> _endQueries;
	std::list<double> _samples;
	double _avg;
	UINT32 _initialized;
	Timer *_timer;

	void Init();
	void Uninit();
};

/**
this class is used for timing
*/
class Timer: public ITimer, public IRenderTimer
{
	friend class GPUPerformanceCounter;
	friend class CPUPerformanceCounter;
public:
	static MGDFError TryCreate( Timer **timer );
	virtual ~Timer( void );

	LARGE_INTEGER GetCurrentTimeTicks() const override final;
	LARGE_INTEGER GetTimerFrequency() const override final;
	double ConvertDifferenceToSeconds( LARGE_INTEGER newTime, LARGE_INTEGER oldTime ) const override final;

	MGDFError CreateCPUCounter( const char *name, IPerformanceCounter **counter ) override final;
	MGDFError CreateGPUCounter( const char *name, IPerformanceCounter **counter ) override final;

	void InitFromDevice( ID3D11Device *device, UINT32 bufferSize, INT32 frameSamples );

	void Begin();
	void End();
	void GetCounterInformation( std::wstringstream &outputStream ) const;

private:
	Timer();
	MGDFError Init();
	void UninitGPUTimer();

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;
	LARGE_INTEGER _freq;
	std::vector<ID3D11Query *> _disjointQueries;

	UINT32 _currentFrame;
	UINT32 _bufferSize;
	UINT32 _maxSamples;
	UINT32 _initialized;
	bool _gpuTimersSupported;

	mutable std::mutex _mutex;
	std::vector<CPUPerformanceCounter *> _cpuCounters;
	std::vector<GPUPerformanceCounter *> _gpuCounters;
	void RemoveCounter( IPerformanceCounter *counter );
};

}
}
