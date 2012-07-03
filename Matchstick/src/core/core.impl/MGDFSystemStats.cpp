#include "StdAfx.h"
#include "MGDFSystemStats.hpp"


namespace MGDF { namespace core {

	SystemStats::SystemStats(unsigned int maxSamples)
		: _avgActiveRenderTime(0)
		, _avgRenderTime(0)
		, _avgActiveSimTime(0)
		, _avgSimTime(0)
		, _avgSimInputTime(0)
		, _avgSimAudioTime(0)
	{
		_maxSamples = maxSamples;
	}

	double SystemStats::AvgActiveRenderTime()
	{
		return _avgActiveRenderTime;
	}

	double SystemStats::AvgRenderTime()
	{
		return _avgRenderTime;
	}

	double SystemStats::AvgActiveSimTime()
	{
		return _avgActiveSimTime;
	}

	double SystemStats::AvgSimTime()
	{
		return _avgSimTime;
	}

	double SystemStats::AvgSimInputTime()
	{
		return _avgSimInputTime;
	}

	double SystemStats::AvgSimAudioTime()
	{
		return _avgSimAudioTime;
	}

	double SystemStats::ActiveRenderTime()
	{
		return *_activeRenderTime.begin();
	}

	double SystemStats::RenderTime()
	{
		return *_renderTime.begin();
	}

	double SystemStats::ExpectedSimTime()
	{
		return _expectedSimTime;
	}

	double SystemStats::ActiveSimTime()
	{
		return *_activeSimTime.begin();
	}

	double SystemStats::SimTime()
	{
		return *_simTime.begin();
	}

	double SystemStats::SimInputTime()
	{
		return *_simInputTime.begin();
	}

	double SystemStats::SimAudioTime()
	{
		return *_simAudioTime.begin();
	}

	std::list<double> *SystemStats::ActiveRenderTimeSamples()
	{
		return &_activeRenderTime;
	}

	std::list<double> *SystemStats::RenderTimeSamples()
	{
		return &_renderTime;
	}

	std::list<double> *SystemStats::ActiveSimTimeSamples()
	{
		return &_activeSimTime;
	}

	std::list<double> *SystemStats::SimTimeSamples()
	{
		return &_simTime;
	}

	std::list<double> *SystemStats::SimInputTimeSamples()
	{
		return &_simInputTime;
	}

	std::list<double> *SystemStats::SimAudioTimeSamples()
	{
		return &_simAudioTime;
	}

	void SystemStats::AppendActiveRenderTime(double value)
	{
		Append(value,_avgActiveRenderTime,_activeRenderTime);
	}

	void SystemStats::AppendRenderTime(double value)
	{
		Append(value,_avgRenderTime,_renderTime);
	}

	void SystemStats::SetExpectedSimTime(double value)
	{
		_expectedSimTime = value;
	}

	void SystemStats::AppendActiveSimTime(double value)
	{
		Append(value,_avgActiveSimTime,_activeSimTime);
	}

	void SystemStats::AppendSimTime(double value)
	{
		Append(value,_avgSimTime,_simTime);
	}

	void SystemStats::AppendSimInputTime(double value)
	{
		Append(value,_avgSimInputTime,_simInputTime);
	}

	void SystemStats::AppendSimAudioTime(double value)
	{
		Append(value,_avgSimAudioTime,_simAudioTime);
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