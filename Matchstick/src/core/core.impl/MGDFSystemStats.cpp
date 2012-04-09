#include "StdAfx.h"
#include "MGDFSystemStats.hpp"


namespace MGDF { namespace core {

	SystemStats::SystemStats(unsigned int maxSamples)
	{
		_maxSamples = maxSamples;
	}

	double SystemStats::AvgActiveRenderTime()
	{
		return GetAverage(_activeRenderTime);
	}

	double SystemStats::AvgRenderTime()
	{
		return GetAverage(_renderTime);
	}

	double SystemStats::AvgActiveSimTime()
	{
		return GetAverage(_activeSimTime);
	}

	double SystemStats::AvgSimTime()
	{
		return GetAverage(_simTime);
	}

	double SystemStats::AvgSimInputTime()
	{
		return GetAverage(_simInputTime);
	}

	double SystemStats::AvgSimAudioTime()
	{
		return GetAverage(_simAudioTime);
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
		Append(value,_activeRenderTime);
	}

	void SystemStats::AppendRenderTime(double value)
	{
		Append(value,_renderTime);
	}

	void SystemStats::SetExpectedSimTime(double value)
	{
		_expectedSimTime = value;
	}

	void SystemStats::AppendActiveSimTime(double value)
	{
		Append(value,_activeSimTime);
	}

	void SystemStats::AppendSimTime(double value)
	{
		Append(value,_simTime);
	}

	void SystemStats::AppendSimInputTime(double value)
	{
		Append(value,_simInputTime);
	}

	void SystemStats::AppendSimAudioTime(double value)
	{
		Append(value,_simAudioTime);
	}

	void SystemStats::Append(double value,std::list<double> &list)
	{
		list.push_front(value);
		if (list.size()>=_maxSamples)
		{
			list.pop_back();
		}
	}

	double SystemStats::GetAverage(std::list<double> &list)
	{
		if (list.size()==0) return 0;

		double total=0;
		for (std::list<double>::iterator iter=list.begin();iter!=list.end();++iter)
		{
			total += (*iter);
		}
		return total/list.size();
	}

}}