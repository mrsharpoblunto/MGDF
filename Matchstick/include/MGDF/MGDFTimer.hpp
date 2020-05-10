#pragma once

#include <MGDF/MGDFPerformanceCounter.hpp>

namespace MGDF {

/**
 Provides an interface to the hosts high resolution CPU timer
 */
class ITimer {
 public:
  /**
  Get the current system time in ticks
  \return the current system time in ticks
  */
  virtual LARGE_INTEGER GetCurrentTimeTicks() const = 0;

  /**
  Get the current system timer frequency
  \return the current system timer frequency
  */
  virtual LARGE_INTEGER GetTimerFrequency() const = 0;

  /**
  Convert two system tick times into a duration in seconds
  \param newTime the newer of the two times
  \param oldTime the older of the two times
  \return the difference betweeen the two parameters converted into seconds
  */
  virtual double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,
                                            LARGE_INTEGER oldTime) const = 0;

  /**
   create a performance counter for profiling CPU time taken. When no longer
   used it should be Released \param name the name of the counter \param counter
   points to the created counter \return MGDF_OK if the counter could be
   created, otherwise an error code is returned
   */
  virtual MGDFError CreateCPUCounter(const char *name,
                                     IPerformanceCounter **counter) = 0;
};

/**
 this provides an interface to Direct3D GPU timers
 */
class IRenderTimer {
 public:
  /**
   * create a performance counter for profiling GPU time taken in DirectX API
   * calls. When no longer used it should be Released \param name the name of
   * the counter \param counter points to the created counter \return
   * MGDF_ERR_GPU_TIMER_UNSUPPORTED if GPU counters are unsupported (using
   * D3D_FEATURE_LEVEL_9_3 or less) otherwise returns MGDF_OK
   */
  virtual MGDFError CreateGPUCounter(const char *name,
                                     IPerformanceCounter **counter) = 0;
};

}  // namespace MGDF