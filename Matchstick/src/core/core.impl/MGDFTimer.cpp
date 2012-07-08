#include "StdAfx.h"

#include <math.h>
#include <mmsystem.h>
#include "MGDFTimer.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFExceptions.hpp"

namespace MGDF { namespace core {

/** ------------CPU counter ---------*/

	
CPUPerformanceCounter::~CPUPerformanceCounter()
{
	_timer->DoRemoveCounter(this);
}

CPUPerformanceCounter::CPUPerformanceCounter(const char *name,Timer *timer)
	: _name(name)
	, _avg(0)
	, _timer(timer)
{
}

void CPUPerformanceCounter::Dispose()
{
	delete this;
}

const char *CPUPerformanceCounter::GetName() const
{
	return _name.c_str();
}

void CPUPerformanceCounter::Begin()
{
	boost::mutex::scoped_lock lock(_mutex);

	QueryPerformanceCounter(&_start);
}

void CPUPerformanceCounter::End()
{
	boost::mutex::scoped_lock lock(_mutex);

	LARGE_INTEGER newTime;
	QueryPerformanceCounter(&newTime);

	LONGLONG diff = newTime.QuadPart - _start.QuadPart;

	double value = (double)diff/_timer->_freq.QuadPart;
	_avg += value/ _timer->_maxSamples;
	_samples.push_front(value);
	if (_samples.size()>= _timer->_maxSamples)
	{
		_avg -= _samples.back() /  _timer->_maxSamples;
		_samples.pop_back();
	}
}

double CPUPerformanceCounter::GetAvgValue()
{
	boost::mutex::scoped_lock lock(_mutex);
	return _avg;
}

/** ------------GPU counter ---------*/

GPUPerformanceCounter::~GPUPerformanceCounter()
{
	_timer->DoRemoveCounter(this);
	Uninit();
}

GPUPerformanceCounter::GPUPerformanceCounter(const char *name,Timer *timer)
	: _name(name)
	, _timer(timer)
	, _avg(0)
	, _initialized(0)
{
	Init();
}

void GPUPerformanceCounter::Dispose()
{
	delete this;
}

const char *GPUPerformanceCounter::GetName() const
{
	return _name.c_str();
}

void GPUPerformanceCounter::Begin()
{
	_beginQueries[_timer->_currentFrame]->Issue(D3DISSUE_END);
}

void GPUPerformanceCounter::End()
{
	_endQueries[_timer->_currentFrame]->Issue(D3DISSUE_END);
	if (_initialized<_timer->_bufferSize)
	{
		_initialized++;
	}
}

void GPUPerformanceCounter::Init()
{
	for (unsigned int i=0;i<_timer->_bufferSize;++i)
	{
		ID3D11Query* query;
		_timer->_device->GetImmediateContext();(D3DQUERYTYPE_TIMESTAMP, &query);
		_beginQueries.push_back(query);
		_timer->_device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &query);
		_endQueries.push_back(query);
	}
}

void GPUPerformanceCounter::Uninit()
{
	for (std::vector<ID3D11Query *>::iterator iter = _beginQueries.begin();iter!=_beginQueries.end();++iter)
	{
		SAFE_RELEASE(*iter);
	}
	for (std::vector<ID3D11Query *>::iterator iter = _endQueries.begin();iter!=_endQueries.end();++iter)
	{
		SAFE_RELEASE(*iter);
	}
	_beginQueries.clear();
	_endQueries.clear();
}

double GPUPerformanceCounter::GetAvgValue()
{
	return _avg;
}

void GPUPerformanceCounter::SetSample(unsigned int frame, UINT64 frequency)
{
	if (_initialized==_timer->_bufferSize)
	{
		UINT64 timeStampBegin;
		if (_beginQueries[frame]->GetData(&timeStampBegin,sizeof(UINT64),0) != S_OK)
		{
			return;
		}	

		UINT64 timeStampEnd;
		if (_endQueries[frame]->GetData(&timeStampEnd,sizeof(UINT64),0) != S_OK)
		{
			return;
		}	

		UINT64 diff = timeStampEnd - timeStampBegin;

		double value = ((double)diff/frequency);
		_avg += value/ _timer->_maxSamples;
		_samples.push_front(value);
		if (_samples.size()>=_timer->_maxSamples)
		{
			_avg -= _samples.back() / _timer->_maxSamples;
			_samples.pop_back();
		}
	}
}

/** -------------- Timer ------------*/
Timer::Timer()
: _currentFrame(0)
, _device(NULL)
, _bufferSize(0)
, _maxSamples(0)
, _initialized(0)
{
	timeBeginPeriod(1);//set a higher resolution for timing calls

    // exit if the system does not support a high performance timer
	if (!QueryPerformanceFrequency(&_freq)) {
		GetLoggerImpl()->Add(THIS_NAME,"High performance timer unsupported",LOG_ERROR);
		throw MGDFException("High performance timer unsupported");
	}
}

Timer::~Timer(void)
{
	timeEndPeriod(1);
	Uninit();
}

void Timer::InitGPUTimer(ID3D11Device *device,unsigned int bufferSize,int frameSamples)
{
	//determine if the queries we need for timing are supported
	HRESULT tsHr = device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, NULL);
	HRESULT tsdHr = device->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, NULL);
	HRESULT tsfHr = device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, NULL);

	if (tsHr || tsdHr || tsfHr)
	{
		GetLoggerImpl()->Add(THIS_NAME,"GPU timestamp queries unsupported",LOG_LOW);
	}
	else
	{
		_device = device;
		_bufferSize = bufferSize;
		_maxSamples = frameSamples;
		Init();
	}
}

void Timer::Init()
{
	for (unsigned int i=0;i<_bufferSize;++i)
	{
		ID3D11Query* query;
		_device->CreateQuery(D3D11_QUERY_TIMESTAMP_DISJOINT, &query);
		_disjointQueries.push_back(query);
	}
}

void Timer::Uninit()
{
	for (std::vector<ID3D11Query *>::iterator iter = _disjointQueries.begin();iter!=_disjointQueries.end();++iter)
	{
		SAFE_RELEASE(*iter);
	}
	_disjointQueries.clear();
}


LARGE_INTEGER Timer::GetCurrentTimeTicks() const
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

LARGE_INTEGER Timer::GetTimerFrequency() const
{
	return _freq;
}

double Timer::ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime) const
{
	LONGLONG diff = newTime.QuadPart - oldTime.QuadPart;
	return (double)diff/_freq.QuadPart;
}

IPerformanceCounter *Timer::CreateCPUCounter(const char *name)
{
	boost::mutex::scoped_lock lock(_mutex);

	CPUPerformanceCounter *counter = new CPUPerformanceCounter(name,this);
	_cpuCounters.push_back(counter);
	return counter;
}

IPerformanceCounter *Timer::CreateGPUCounter(const char *name)
{
	if (_device)
	{
		GPUPerformanceCounter *counter = new GPUPerformanceCounter(name,this);
		_gpuCounters.push_back(counter);
		return counter;
	}
	return NULL;
}

void Timer::RemoveCounter(IPerformanceCounter *counter)
{
	delete counter;
}

void Timer::DoRemoveCounter(IPerformanceCounter *counter)
{
	{
		boost::mutex::scoped_lock lock(_mutex);

		for (std::vector<CPUPerformanceCounter *>::iterator iter = _cpuCounters.begin();iter!=_cpuCounters.end();++iter)
		{
			if (*iter==counter)
			{
				_cpuCounters.erase(iter);
				return;
			}
		}
	}

	for (std::vector<GPUPerformanceCounter *>::iterator iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
	{
		if (*iter==counter)
		{
			_gpuCounters.erase(iter);
			return;
		}
	}
}

void Timer::Begin()
{
	if (_device)
	{
		int previousFrame = _currentFrame; 
		_currentFrame = (_currentFrame+1) % _bufferSize;

		if (_initialized == _bufferSize)
		{
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
			if (_disjointQueries[previousFrame]->GetData(&disjoint,sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT),0) == S_OK)
			{
				if (!disjoint.Disjoint)
				{
					for (std::vector<GPUPerformanceCounter *>::iterator iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
					{
						(*iter)->SetSample(previousFrame,disjoint.Frequency);
					}
				}
			}
		}
		else
		{
			++_initialized;
		}

		_disjointQueries[_currentFrame]->Issue(D3DISSUE_BEGIN);
	}
}

void Timer::End()
{
	if (_device)
	{
		_disjointQueries[_currentFrame]->Issue(D3DISSUE_END);
	}
}

void Timer::GetCounterAverages(
		std::vector<std::pair<const char *,double> > &cpuCounters,
		std::vector<std::pair<const char *,double> > &gpuCounters)
{
	for (std::vector<CPUPerformanceCounter *>::iterator iter = _cpuCounters.begin();iter!=_cpuCounters.end();++iter)
	{
		cpuCounters.push_back(std::pair<const char *,double>((*iter)->GetName(),(*iter)->GetAvgValue()));
	}
	for (std::vector<GPUPerformanceCounter *>::iterator iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
	{
		gpuCounters.push_back(std::pair<const char *,double>((*iter)->GetName(),(*iter)->GetAvgValue()));
	}
}

}}