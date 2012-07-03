#pragma once

#include <list>

namespace MGDF { namespace core {

class SystemStats
{
public:
	SystemStats(unsigned int maxSamples);
	virtual ~SystemStats(){};

	double ActiveRenderTime();
	double RenderTime();
	double ExpectedSimTime();
	double ActiveSimTime();
	double SimTime();
	double SimInputTime();
	double SimAudioTime();

	double AvgActiveRenderTime();
	double AvgRenderTime();
	double AvgActiveSimTime();
	double AvgSimTime();
	double AvgSimInputTime();
	double AvgSimAudioTime();

	std::list<double> *ActiveRenderTimeSamples();
	std::list<double> *RenderTimeSamples();
	std::list<double> *ActiveSimTimeSamples();
	std::list<double> *SimTimeSamples();
	std::list<double> *SimInputTimeSamples();
	std::list<double> *SimAudioTimeSamples();

	void AppendActiveRenderTime(double value);
	void AppendRenderTime(double value);
	void SetExpectedSimTime(double value);
	void AppendActiveSimTime(double value);
	void AppendSimTime(double value);
	void AppendSimInputTime(double value);
	void AppendSimAudioTime(double value);
private:
	unsigned int _maxSamples;
	double _expectedSimTime;

	std::list<double> _activeRenderTime;
	std::list<double> _renderTime;
	std::list<double> _activeSimTime;
	std::list<double> _simTime;
	std::list<double> _simInputTime;
	std::list<double> _simAudioTime;	

	double _avgActiveRenderTime;
	double _avgRenderTime;
	double _avgActiveSimTime;
	double _avgSimTime;
	double _avgSimInputTime;
	double _avgSimAudioTime;

	void Append(double value,double &average,std::list<double> &list);
};

}}