#pragma once

#include <MGDF/MGDFPerformanceCounter.hpp>

namespace MGDF {

/**
 Provides an interface to the hosts high resolution CPU timer
 */
MIDL_INTERFACE("4DA26ED2-514E-4CB2-B9D4-6AC4170DFDCE")
ITimer : public IUnknown {
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
};

}  // namespace MGDF