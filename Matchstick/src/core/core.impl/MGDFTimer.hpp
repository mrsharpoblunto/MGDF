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

	void Dispose() override;
	const char *GetName() const override;
	void Begin() override;
	void End() override;

	double GetAvgValue();
private:
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

	void Dispose() override;
	const char *GetName() const override;
	void Begin() override;
	void End() override;

	void Reinit();
	double GetAvgValue();
	void SetSample( UINT32 previousFrame, UINT64 frequency );
private:
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
	Timer();
	virtual ~Timer( void );

	LARGE_INTEGER GetCurrentTimeTicks() const override;
	LARGE_INTEGER GetTimerFrequency() const override;
	double ConvertDifferenceToSeconds( LARGE_INTEGER newTime, LARGE_INTEGER oldTime ) const override;

	MGDFError CreateCPUCounter( const char *name, IPerformanceCounter **counter ) override;
	MGDFError CreateGPUCounter( const char *name, IPerformanceCounter **counter ) override;

	void InitFromDevice( ID3D11Device *device, UINT32 bufferSize, INT32 frameSamples );

	void Begin();
	void End();
	void GetCounterInformation( std::wstringstream &outputStream ) const;

private:
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
