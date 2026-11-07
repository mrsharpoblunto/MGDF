#include "StdAfx.h"

#include "TextManager.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

TextManagerState::TextManagerState(const TextManagerState &,
                                   const TextManagerState &endState, double)
    : _lastLine(endState._lastLine) {}
TextManagerState::TextManagerState(const TextManagerState &state) = default;
std::function<void(const std::string &)> TextManagerState::LogSink;
void TextManagerState::AddLine(const std::string &line) {
  _lastLine = line;
  if (LogSink && !line.empty()) LogSink(line);
}
void TextManagerState::SetStatus(TextColor, const std::string &text) {
  if (LogSink) LogSink(_lastLine + " " + text);
}
}  // namespace Test
}  // namespace MGDF
