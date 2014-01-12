#include "StdAfx.h"

#include <math.h>
#include <mmsystem.h>
#include "MGDFTimer.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFExceptions.hpp"

#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace core
{

/** ------------CPU counter ---------*/
CPUPerformanceCounter::~CPUPerformanceCounter()
{
	_timer->RemoveCounter( this );
}

CPUPerformanceCounter::CPUPerformanceCounter( const char *name, Timer *timer )
	: _name( name )
	, _avg( 0 )
	, _timer( timer )
{
	_ASSERTE( name );
	_ASSERTE( timer );
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
	std::lock_guard<std::mutex> lock( _mutex );

	QueryPerformanceCounter( &_start );
}

void CPUPerformanceCounter::End()
{
	std::lock_guard<std::mutex> lock( _mutex );

	LARGE_INTEGER newTime;
	QueryPerformanceCounter( &newTime );

	LONGLONG diff = newTime.QuadPart - _start.QuadPart;

	double value = ( double ) diff / _timer->_freq.QuadPart;
	_avg += value / _timer->_maxSamples;
	_samples.push_front( value );
	if ( _samples.size() >= _timer->_maxSamples ) {
		_avg -= _samples.back() /  _timer->_maxSamples;
		_samples.pop_back();
	}
}

double CPUPerformanceCounter::GetAvgValue()
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _avg;
}

/** ------------GPU counter ---------*/

GPUPerformanceCounter::~GPUPerformanceCounter()
{
	_timer->RemoveCounter( this );
	Uninit();
}

GPUPerformanceCounter::GPUPerformanceCounter( const char *name, Timer *timer )
	: _name( name )
	, _timer( timer )
	, _avg( 0 )
	, _initialized( 0 )
{
	_ASSERTE( name );
	_ASSERTE( timer );
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
	_timer->_context->End( _beginQueries[_timer->_currentFrame] );
}

void GPUPerformanceCounter::End()
{
	_timer->_context->End( _endQueries[_timer->_currentFrame] );
	if ( _initialized < _timer->_bufferSize ) {
		_initialized++;
	}
}

void GPUPerformanceCounter::Reinit()
{
	_avg = 0;
	_initialized = 0;
	_samples.clear();
	Uninit();
	Init();
}

void GPUPerformanceCounter::Init()
{
	for ( UINT32 i = 0; i < _timer->_bufferSize; ++i ) {
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_TIMESTAMP;
		desc.MiscFlags = 0;

		ID3D11Query* query;
		_timer->_device->CreateQuery( &desc, &query );
		_beginQueries.push_back( query );
		_timer->_device->CreateQuery( &desc, &query );
		_endQueries.push_back( query );
	}
}

void GPUPerformanceCounter::Uninit()
{
	for ( auto iter = _beginQueries.begin(); iter != _beginQueries.end(); ++iter ) {
		SAFE_RELEASE( *iter );
	}
	for ( auto iter = _endQueries.begin(); iter != _endQueries.end(); ++iter ) {
		SAFE_RELEASE( *iter );
	}
	_beginQueries.clear();
	_endQueries.clear();
}

double GPUPerformanceCounter::GetAvgValue()
{
	return _avg;
}

void GPUPerformanceCounter::SetSample( UINT32 frame, UINT64 frequency )
{
	if ( _initialized == _timer->_bufferSize ) {
		UINT64 timeStampBegin;
		if ( _timer->_context->GetData( _beginQueries[frame], &timeStampBegin, sizeof( UINT64 ), 0 ) != S_OK ) {
			return;
		}

		UINT64 timeStampEnd;
		if ( _timer->_context->GetData( _endQueries[frame], &timeStampEnd, sizeof( UINT64 ), 0 ) != S_OK ) {
			return;
		}

		UINT64 diff = timeStampEnd - timeStampBegin;

		double value = ( ( double ) diff / frequency );
		_avg += value / _timer->_maxSamples;
		_samples.push_front( value );
		if ( _samples.size() >= _timer->_maxSamples ) {
			_avg -= _samples.back() / _timer->_maxSamples;
			_samples.pop_back();
		}
	}
}

/** -------------- Timer ------------*/
Timer::Timer()
	: _currentFrame( 0 )
	, _device( nullptr )
	, _context( nullptr )
	, _bufferSize( 0 )
	, _maxSamples( 0 )
	, _initialized( 0 )
	, _gpuTimersSupported( true )
{
	timeBeginPeriod( 1 );  //set a higher resolution for timing calls

	// exit if the  does not support a high performance timer
	if ( !QueryPerformanceFrequency( &_freq ) ) {
		LOG( "High performance timer unsupported", LOG_ERROR );
		throw MGDFException( "High performance timer unsupported" );
	}
}

Timer::~Timer( void )
{
	timeEndPeriod( 1 );

	while ( _cpuCounters.size() > 0 ) {
		delete _cpuCounters.back();
	}
	while ( _gpuCounters.size() > 0 ) {
		delete _gpuCounters.back();
	}

	UninitGPUTimer();
}

void Timer::UninitGPUTimer()
{
	
	for ( auto iter = _disjointQueries.begin(); iter != _disjointQueries.end(); ++iter ) {
		SAFE_RELEASE( *iter );
	}
	_disjointQueries.clear();
	SAFE_RELEASE( _context );
}


void Timer::InitFromDevice( ID3D11Device *device, UINT32 bufferSize, INT32 frameSamples )
{
	_ASSERTE( device );

	if ( _device ) {
		// if a device is reset, then we need to 
		// clean up any old counters and contexts
		UninitGPUTimer();
	}

	_device = device;
	device->GetImmediateContext( &_context );
	_bufferSize = bufferSize;
	_maxSamples = frameSamples;

	for ( UINT32 i = 0; i < _bufferSize; ++i ) {
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		desc.MiscFlags = 0;

		ID3D11Query* query;
		_device->CreateQuery( &desc, &query );
		if ( !query ) {
			LOG( "GPU timing queries unsupported", LOG_ERROR );
			_gpuTimersSupported = false;
			break;
		}
		_disjointQueries.push_back( query );
	}

	for (auto gpuCounter : _gpuCounters ) {
		gpuCounter->Reinit();
	}
}

LARGE_INTEGER Timer::GetCurrentTimeTicks() const
{
	LARGE_INTEGER result;
	QueryPerformanceCounter( &result );
	return result;
}

LARGE_INTEGER Timer::GetTimerFrequency() const
{
	return _freq;
}

double Timer::ConvertDifferenceToSeconds( LARGE_INTEGER newTime, LARGE_INTEGER oldTime ) const
{
	LONGLONG diff = newTime.QuadPart - oldTime.QuadPart;
	return ( double ) diff / _freq.QuadPart;
}

MGDFError Timer::CreateCPUCounter( const char *name, IPerformanceCounter **counter )
{
	if ( !name ) return MGDF_ERR_INVALID_TIMER_NAME;

	CPUPerformanceCounter *c = new CPUPerformanceCounter( name, this );

	std::lock_guard<std::mutex> lock( _mutex );
	_cpuCounters.push_back( c );
	*counter = c;
	return MGDF_OK;
}

MGDFError Timer::CreateGPUCounter( const char *name, IPerformanceCounter **counter )
{
	if ( !_gpuTimersSupported) return MGDF_ERR_GPU_TIMER_UNSUPPORTED;
	if (!name ) return MGDF_ERR_INVALID_TIMER_NAME;
	GPUPerformanceCounter *c = new GPUPerformanceCounter( name, this );

	std::lock_guard<std::mutex> lock( _mutex );
	_gpuCounters.push_back( c );
	*counter = c;
	return MGDF_OK;
}

void Timer::RemoveCounter( IPerformanceCounter *counter )
{
	if ( !counter ) return;
	CPUPerformanceCounter *cpuCounter = dynamic_cast<CPUPerformanceCounter *>( counter );

	std::lock_guard<std::mutex> lock( _mutex );
	if ( cpuCounter ) {
		for ( auto iter = _cpuCounters.begin(); iter != _cpuCounters.end(); ++iter ) {
			if ( *iter == counter ) {
				_cpuCounters.erase( iter );
				return;
			}
		}
	} else {
		for ( auto iter = _gpuCounters.begin(); iter != _gpuCounters.end(); ++iter ) {
			if ( *iter == counter ) {
				_gpuCounters.erase( iter );
				return;
			}
		}
	}
}

void Timer::Begin()
{
	_currentFrame = ( _currentFrame + 1 ) % _bufferSize;

	if ( _gpuTimersSupported ) {
		if ( _initialized == _bufferSize ) {
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;

			if ( _context->GetData( _disjointQueries[_currentFrame], &disjoint, sizeof( D3D11_QUERY_DATA_TIMESTAMP_DISJOINT ), 0 ) == S_OK ) {
				if ( !disjoint.Disjoint ) {
					std::lock_guard<std::mutex> lock( _mutex );
					for ( auto iter = _gpuCounters.begin(); iter != _gpuCounters.end(); ++iter ) {
						( *iter )->SetSample( _currentFrame, disjoint.Frequency );
					}
				}
			}
		} else {
			++_initialized;
		}

		_context->Begin( _disjointQueries[_currentFrame] );
	}
}

void Timer::End()
{
	if ( _gpuTimersSupported ) _context->End( _disjointQueries[_currentFrame] );
}

void Timer::GetCounterInformation( std::wstringstream &outputStream ) const
{
	std::lock_guard<std::mutex> lock( _mutex );

	if ( _gpuCounters.size() > 0 ) {
		outputStream << "\r\nGPU\r\n";
		for ( auto counter : _gpuCounters ) {
			outputStream << " " << counter->GetName() << " : " << counter->GetAvgValue() << "\r\n";
		}
	}

	if ( _cpuCounters.size() > 0 ) {
		outputStream << "\r\nCPU\r\n";
		for ( auto counter : _cpuCounters ) {
			outputStream << " " << counter->GetName() << " : " << counter->GetAvgValue() << "\r\n";
		}
	}
}

}
}