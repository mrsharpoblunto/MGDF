#pragma once

#include "d3d11.h"
#include <vector>
#include <MGDF/MGDFTimer.hpp>
#include <boost/thread/thread.hpp>

namespace MGDF { namespace core {

class Timer;

class CPUPerformanceCounter: public DisposeImpl<IPerformanceCounter>
{
public:
	virtual ~CPUPerformanceCounter();
	CPUPerformanceCounter(const char *name,Timer *timer);

	virtual void Dispose();
	virtual const char *GetName() const;
	virtual void Begin();
	virtual void End();

	double GetAvgValue();
private:
	boost::mutex _mutex;
	std::string _name;
	LARGE_INTEGER _start;
	std::list<double> _samples;
	double _avg;
	Timer *_timer;

};

class GPUPerformanceCounter: public DisposeImpl<IPerformanceCounter>
{
public:
	virtual ~GPUPerformanceCounter();
	GPUPerformanceCounter(const char *name,Timer *timer);

	virtual void Dispose();
	virtual const char *GetName() const;
	virtual void Begin();
	virtual void End();

	double GetAvgValue();
	void SetSample(UINT32 previousFrame, UINT64 frequency);
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
class Timer: public ITimer
{
friend class GPUPerformanceCounter;
friend class CPUPerformanceCounter;
public:
	Timer();
	virtual ~Timer(void);

	virtual LARGE_INTEGER GetCurrentTimeTicks() const;
	virtual LARGE_INTEGER GetTimerFrequency() const;
	virtual double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime) const;

	virtual IPerformanceCounter *CreateCPUCounter(const char *name);
	virtual IPerformanceCounter *CreateGPUCounter(const char *name);
	virtual void RemoveCounter(IPerformanceCounter *counter);

	void InitGPUTimer(ID3D11Device *device,UINT32 bufferSize,INT32 frameSamples);

	void Begin();
	void End();
	void GetCounterAverages(
		std::vector<std::pair<const char *,double> > &cpuCounters,
		std::vector<std::pair<const char *,double> > &gpuCounters);

	boost::mutex &Mutex() { return _mutex; }

private:
	ID3D11Device *_device;
	ID3D11DeviceContext *_context;
	LARGE_INTEGER _freq;
	std::vector<ID3D11Query *> _disjointQueries;

	UINT32 _currentFrame;
	UINT32 _bufferSize;
	UINT32 _maxSamples;
	UINT32 _initialized;
	bool _gpuTimersSupported;

	boost::mutex _mutex;
	std::vector<CPUPerformanceCounter *> _cpuCounters;
	std::vector<GPUPerformanceCounter *> _gpuCounters;

	void DoRemoveCounter(IPerformanceCounter *counter);
};

}}
