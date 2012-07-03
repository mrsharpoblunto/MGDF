#pragma once

#include "d3d9.h"
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

	void OnLostDevice();
	void OnResetDevice();
	double GetAvgValue();
	void SetSample(unsigned int previousFrame, UINT64 frequency);
private:
	std::string _name;
	std::vector<IDirect3DQuery9 *> _beginQueries;
	std::vector<IDirect3DQuery9 *> _endQueries;
	std::list<double> _samples;
	double _avg;
	unsigned int _initialized;
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

	void InitGPUTimer(IDirect3DDevice9 *device,unsigned int bufferSize,int frameSamples);
	void OnLostDevice();
	void OnResetDevice();

	void Begin();
	void End();
	void GetCounterAverages(
		std::vector<std::pair<const char *,double> > &cpuCounters,
		std::vector<std::pair<const char *,double> > &gpuCounters);

	boost::mutex &Mutex() { return _mutex; }

private:
	IDirect3DDevice9 *_device;
	LARGE_INTEGER _freq;
	std::vector<IDirect3DQuery9 *> _disjointQueries;
	std::vector<IDirect3DQuery9 *> _frequencyQueries;
	unsigned int _currentFrame;
	unsigned int _bufferSize;
	unsigned int _maxSamples;
	unsigned int _initialized;
	boost::mutex _mutex;
	std::vector<CPUPerformanceCounter *> _cpuCounters;
	std::vector<GPUPerformanceCounter *> _gpuCounters;

	void Init();
	void Uninit();
	void DoRemoveCounter(IPerformanceCounter *counter);
};

}}
