#pragma once

#include <MGDF/MGDF.h>
#include <dwrite_1.h>

#include <MGDF/ComObject.hpp>

#include "Common.hpp"

namespace MGDF {
namespace Test {

enum class TextColor {
  GREEN,
  RED,
};

struct Line {
  std::string Content;

  TextColor StatusColor;
  std::string StatusText;
  Line() : StatusColor(TextColor::GREEN) {}
};

class TextManager;

class TESTCOMMON_DLL TextManagerState {
  friend class TextManager;

 public:
  virtual ~TextManagerState() {};
  TextManagerState() {};
  TextManagerState(const TextManagerState &state);
  TextManagerState(const TextManagerState &startState,
                   const TextManagerState &endState, double alpha);
  void AddLine(const std::string &line);
  void SetStatus(TextColor color, const std::string &text);

 private:
#pragma warning(push)
#pragma warning(disable : 4251)
  std::vector<Line> _lines;
#pragma warning(pop)
};

class TESTCOMMON_DLL TextManager {
 public:
  virtual ~TextManager();
  TextManager(IMGDFRenderHost *renderHost);
  void SetState(TextManagerState &state);
  void BackBufferChange();
  void BeforeBackBufferChange();
  void BeforeDeviceReset();
  void DrawText();

 private:
#pragma warning(push)
#pragma warning(disable : 4251)
  TextManagerState _state;
  ComObject<IMGDFRenderSettingsManager> _settings;
  ComObject<ID2D1SolidColorBrush> _whiteBrush;
  ComObject<ID2D1SolidColorBrush> _redBrush;
  ComObject<ID2D1SolidColorBrush> _greenBrush;
  ComObject<ID2D1DeviceContext> _d2dContext;
  ComObject<IDWriteFactory1> _dWriteFactory;
  ComObject<IDWriteTextFormat> _textFormat;
#pragma warning(pop)
  IMGDFRenderHost *_renderHost;
};
}  // namespace Test
}  // namespace MGDF