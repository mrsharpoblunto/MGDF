#pragma once

#include <d3d11.h>
#include <vector>
#include <list>
#include <mutex>
#include <deque>
#include <stack>
#include <unordered_map>
#include <MGDF/MGDFTimer.hpp>

namespace MGDF
{
namespace core
{

class Timer;

class CounterBase: public IPerformanceCounter
{
public:
	virtual ~CounterBase();
	CounterBase( const char *name, UINT32 maxSamples, Timer &timer );

	const char *GetName() const override final;

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject ) override final;
	ULONG STDMETHODCALLTYPE AddRef() override final;
	ULONG STDMETHODCALLTYPE Release() override final;
	ULONG RefCount() const { return _references; }

	double GetAvgValue() const;
protected:
	void AddSample( double sample );
	bool _started;
private:
	mutable std::mutex _mutex;
	double _avg;

	Timer &_timer;
	ULONG _references;
	std::string _name;
	UINT32 _maxSamples;
	std::deque<double> _samples;
};

class CPUPerformanceCounter: public CounterBase
{
public:
	virtual ~CPUPerformanceCounter();
	CPUPerformanceCounter( const char *name, UINT32 maxSamples, Timer &timer, LARGE_INTEGER frequency );

	void Begin() override final;
	void End() override final;
private:
	LARGE_INTEGER _start;
	LARGE_INTEGER _frequency;
};


class GPUPerformanceCounter: public CounterBase
{
public:
	virtual ~GPUPerformanceCounter();
	GPUPerformanceCounter( const char *name, UINT32 maxSamples, Timer &timer );

	void Begin() override final;
	void End() override final;

	void ForceEnd();
	void Init( ID3D11Device *device, ID3D11DeviceContext *context, UINT bufferSize );
	void Reset();
	void DataReady( ID3D11Query *disjoint, UINT64 frequency );
	void DataDisjoint( ID3D11Query *disjoint );
	void SetDisjointQuery( ID3D11Query *disjoint );
private:
	std::unordered_map<ID3D11Query *,std::pair<ID3D11Query *,ID3D11Query *> > _pendingQueries;
	std::stack<ID3D11Query *> _beginQueries;
	std::stack<ID3D11Query *> _endQueries;
	ID3D11Query *_currentDisjoint;
	ID3D11DeviceContext *_context;
	bool _hasRun;
};

/**
this class is used for timing
*/
class Timer: public ITimer, public IRenderTimer
{
public:
	static MGDFError TryCreate( UINT32 frameSamples, Timer **timer );
	virtual ~Timer( void );

	LARGE_INTEGER GetCurrentTimeTicks() const override final;
	LARGE_INTEGER GetTimerFrequency() const override final;
	double ConvertDifferenceToSeconds( LARGE_INTEGER newTime, LARGE_INTEGER oldTime ) const override final;

	MGDFError CreateCPUCounter( const char *name, IPerformanceCounter **counter ) override final;
	MGDFError CreateGPUCounter( const char *name, IPerformanceCounter **counter ) override final;

	void InitFromDevice( ID3D11Device *device, UINT32 bufferSize );

	void Begin();
	void End();
	void GetCounterInformation( std::wstringstream &outputStream ) const;
	void RemoveCounter( IPerformanceCounter *counter );

private:
	Timer( UINT32 maxSamples );
	MGDFError Init();
	void ResetGPUTimers();

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;
	LARGE_INTEGER _freq;
	
	std::deque<ID3D11Query *> _pendingQueries;
	std::stack<ID3D11Query *> _disjointQueries;
	ID3D11Query *_currentQuery;

	UINT32 _bufferSize;
	UINT32 _maxSamples;
	bool _gpuTimersSupported;

	mutable std::mutex _mutex;
	std::vector<CPUPerformanceCounter *> _cpuCounters;
	std::vector<GPUPerformanceCounter *> _gpuCounters;
};

}
}
