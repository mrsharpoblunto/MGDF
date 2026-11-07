#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <functional>
#include <string>

#include "Common.hpp"

namespace MGDF {
namespace Test {

enum class TextColor {
  GREEN,
  RED,
};

class TESTCOMMON_DLL TextManagerState {
 public:
  virtual ~TextManagerState() {};
  TextManagerState() {};
  TextManagerState(const TextManagerState &state);
  TextManagerState(const TextManagerState &startState,
                   const TextManagerState &endState, double alpha);
  void AddLine(const std::string &line);
  void SetStatus(TextColor color, const std::string &text);
  // When set, all test output lines and status changes are also
  // forwarded to this sink (e.g. to mirror them into the MGDF log)
#pragma warning(push)
#pragma warning(disable : 4251)
  static std::function<void(const std::string &)> LogSink;
#pragma warning(pop)

 private:
#pragma warning(push)
#pragma warning(disable : 4251)
  std::string _lastLine;
#pragma warning(pop)
};

}  // namespace Test
}  // namespace MGDF
