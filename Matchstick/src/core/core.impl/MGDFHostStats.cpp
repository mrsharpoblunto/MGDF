#include "StdAfx.h"
#include "MGDFHostStats.hpp"


namespace MGDF
{
namespace core
{

HostStats::HostStats( UINT32 maxSamples )
	: _avgActiveRenderTime( 0 )
	, _avgRenderTime( 0 )
	, _avgActiveSimTime( 0 )
	, _avgSimTime( 0 )
	, _avgSimInputTime( 0 )
	, _avgSimAudioTime( 0 )
{
	_maxSamples = maxSamples;
}

void HostStats::GetTimings( Timings &timings ) const
{
	boost::mutex::scoped_lock lock( _statsMutex );
	timings.AvgActiveRenderTime = _avgActiveRenderTime;
	timings.AvgRenderTime = _avgRenderTime;
	timings.AvgActiveSimTime = _avgActiveSimTime;
	timings.AvgSimTime = _avgSimTime;
	timings.AvgSimInputTime = _avgSimInputTime;
	timings.AvgSimAudioTime = _avgSimAudioTime;
	timings.ExpectedSimTime = _expectedSimTime;
}

void HostStats::SetExpectedSimTime( double value )
{
	_expectedSimTime = value;
}

double HostStats::ExpectedSimTime() const
{
	return _expectedSimTime;
}

void HostStats::AppendRenderTimes( double renderValue, double activeRenderValue )
{
	boost::mutex::scoped_lock lock( _statsMutex );
	Append( renderValue, _avgRenderTime, _renderTime );
	Append( activeRenderValue, _avgActiveRenderTime, _activeRenderTime );
}

void HostStats::AppendActiveSimTime( double value )
{
	boost::mutex::scoped_lock lock( _statsMutex );
	Append( value - *_simInputTime.begin() - *_simAudioTime.begin(), _avgActiveSimTime, _activeSimTime );
}

void HostStats::AppendSimTime( double value )
{
	boost::mutex::scoped_lock lock( _statsMutex );
	Append( value, _avgSimTime, _simTime );
}

void HostStats::AppendSimInputAndAudioTimes( double inputValue, double audioValue )
{
	boost::mutex::scoped_lock lock( _statsMutex );
	Append( inputValue, _avgSimInputTime, _simInputTime );
	Append( audioValue, _avgSimAudioTime, _simAudioTime );
}

void HostStats::Append( double value, double &averageValue, std::list<double> &list )
{
	list.push_front( value );
	averageValue += ( value / _maxSamples );
	if ( list.size() >= _maxSamples ) {
		averageValue -= ( list.back() / _maxSamples );
		list.pop_back();
	}
}

}
}