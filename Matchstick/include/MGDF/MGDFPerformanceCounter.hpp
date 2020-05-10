#pragma once

namespace MGDF {

/**
 A names performance counter that can be used to time the duration of
 events. The results of these counters are shown in the MGDF stats overlay
 (press ALT-F12 to see in game)
 */
class __declspec(uuid("E2E9FE7E-4D07-40FE-9858-8E583D73CB37"))
    IPerformanceCounter : public IUnknown {
 public:
  /**
   Gets the name of the counter
   \return the name of the counter
   */
  virtual const char *GetName() const = 0;

  /**
   Begin timing
   */
  virtual void Begin() = 0;

  /**
   End timing and record the duration between the time when begin was called
   and when End was called
   */
  virtual void End() = 0;
};

}  // namespace MGDF