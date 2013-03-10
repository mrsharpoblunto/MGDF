#include "StdAfx.h"
#include "MGDFSystemStats.hpp"


namespace MGDF { namespace core {

	SystemStats::SystemStats(UINT32 maxSamples)
		: _avgActiveRenderTime(0)
		, _avgRenderTime(0)
		, _avgActiveSimTime(0)
		, _avgSimTime(0)
		, _avgSimInputTime(0)
		, _avgSimAudioTime(0)
	{
		_maxSamples = maxSamples;
	}

	void SystemStats::GetTimings(Timings &timings)
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		timings.AvgActiveRenderTime = _avgActiveRenderTime;
		timings.AvgRenderTime = _avgRenderTime;
		timings.AvgActiveSimTime = _avgActiveSimTime;
		timings.AvgSimTime = _avgSimTime;
		timings.AvgSimInputTime = _avgSimInputTime;
		timings.AvgSimAudioTime = _avgSimAudioTime;
		timings.ExpectedSimTime = _expectedSimTime;
	}

	void SystemStats::SetExpectedSimTime(double value)
	{
		_expectedSimTime = value;
	}

	double SystemStats::ExpectedSimTime()
	{
		return _expectedSimTime;
	}

	void SystemStats::AppendRenderTimes(double renderValue,double activeRenderValue)
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		Append(renderValue,_avgRenderTime,_renderTime);
		Append(activeRenderValue,_avgActiveRenderTime,_activeRenderTime);
	}

	void SystemStats::AppendActiveSimTime(double value)
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		Append(value - *_simInputTime.begin() - *_simAudioTime.begin(),_avgActiveSimTime,_activeSimTime);
	}

	void SystemStats::AppendSimTime(double value)
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		Append(value,_avgSimTime,_simTime);
	}

	void SystemStats::AppendSimInputAndAudioTimes(double inputValue,double audioValue)
	{
		boost::mutex::scoped_lock lock(_statsMutex);
		Append(inputValue,_avgSimInputTime,_simInputTime);
		Append(audioValue,_avgSimAudioTime,_simAudioTime);
	}

	void SystemStats::Append(double value,double &averageValue,std::list<double> &list)
	{
		list.push_front(value);
		averageValue += (value/_maxSamples);
		if (list.size()>=_maxSamples)
		{
			averageValue -= (list.back()/_maxSamples);
			list.pop_back();
		}
	}

}}