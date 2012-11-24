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
	_timer->_context->End(_beginQueries[_timer->_currentFrame]);
}

void GPUPerformanceCounter::End()
{
	_timer->_context->End(_endQueries[_timer->_currentFrame]);
	if (_initialized<_timer->_bufferSize)
	{
		_initialized++;
	}
}

void GPUPerformanceCounter::Init()
{
	for (UINT32 i=0;i<_timer->_bufferSize;++i)
	{
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_TIMESTAMP;
		desc.MiscFlags = 0;

		ID3D11Query* query;
		_timer->_device->CreateQuery(&desc, &query);
		_beginQueries.push_back(query);
		_timer->_device->CreateQuery(&desc, &query);
		_endQueries.push_back(query);
	}
}

void GPUPerformanceCounter::Uninit()
{
	for (auto iter = _beginQueries.begin();iter!=_beginQueries.end();++iter)
	{
		SAFE_RELEASE(*iter);
	}
	for (auto iter = _endQueries.begin();iter!=_endQueries.end();++iter)
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

void GPUPerformanceCounter::SetSample(UINT32 frame, UINT64 frequency)
{
	if (_initialized==_timer->_bufferSize)
	{
		UINT64 timeStampBegin;
		if (_timer->_context->GetData(_beginQueries[frame],&timeStampBegin,sizeof(UINT64),0) != S_OK)
		{
			return;
		}	

		UINT64 timeStampEnd;
		if (_timer->_context->GetData(_endQueries[frame],&timeStampEnd,sizeof(UINT64),0) != S_OK)
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
, _device(nullptr)
, _context(nullptr)
, _bufferSize(0)
, _maxSamples(0)
, _initialized(0)
, _gpuTimersSupported(true)
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

	for (auto iter = _disjointQueries.begin();iter!=_disjointQueries.end();++iter)
	{
		SAFE_RELEASE(*iter);
	}
	SAFE_RELEASE(_context);
}

void Timer::InitGPUTimer(ID3D11Device *device,UINT32 bufferSize,INT32 frameSamples)
{
	_device = device;
	device->GetImmediateContext(&_context);
	_bufferSize = bufferSize;
	_maxSamples = frameSamples;

	for (UINT32 i=0;i<_bufferSize;++i)
	{
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		desc.MiscFlags = 0;

		ID3D11Query* query;
		_device->CreateQuery(&desc, &query);
		if (!query)
		{
			GetLoggerImpl()->Add(THIS_NAME,"GPU timing queries unsupported",LOG_ERROR);
			_gpuTimersSupported = false;
			break;
		}
		_disjointQueries.push_back(query);
	}
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
	if (!_gpuTimersSupported) return nullptr;
	GPUPerformanceCounter *counter = new GPUPerformanceCounter(name,this);
	_gpuCounters.push_back(counter);
	return counter;
}

void Timer::RemoveCounter(IPerformanceCounter *counter)
{
	delete counter;
}

void Timer::DoRemoveCounter(IPerformanceCounter *counter)
{
	{
		boost::mutex::scoped_lock lock(_mutex);

		for (auto iter = _cpuCounters.begin();iter!=_cpuCounters.end();++iter)
		{
			if (*iter==counter)
			{
				_cpuCounters.erase(iter);
				return;
			}
		}
	}

	for (auto iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
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
	_currentFrame = (_currentFrame+1) % _bufferSize;

	if (_gpuTimersSupported) 
	{
		if (_initialized == _bufferSize)
		{
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
		
			if (_context->GetData(_disjointQueries[_currentFrame],&disjoint,sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT),0) == S_OK)
			{
				if (!disjoint.Disjoint)
				{
					for (auto iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
					{
						(*iter)->SetSample(_currentFrame,disjoint.Frequency);
					}
				}
			}
		}
		else
		{
			++_initialized;
		}

		_context->Begin(_disjointQueries[_currentFrame]);
	}
}

void Timer::End()
{
	if (_gpuTimersSupported) _context->End(_disjointQueries[_currentFrame]);
}

void Timer::GetCounterAverages(
		std::vector<std::pair<const char *,double> > &cpuCounters,
		std::vector<std::pair<const char *,double> > &gpuCounters)
{
	for (auto iter = _cpuCounters.begin();iter!=_cpuCounters.end();++iter)
	{
		cpuCounters.push_back(std::pair<const char *,double>((*iter)->GetName(),(*iter)->GetAvgValue()));
	}
	for (auto iter = _gpuCounters.begin();iter!=_gpuCounters.end();++iter)
	{
		gpuCounters.push_back(std::pair<const char *,double>((*iter)->GetName(),(*iter)->GetAvgValue()));
	}
}

}}