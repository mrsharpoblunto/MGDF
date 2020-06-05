#pragma once

#include <d3d11.h>
#include <dwrite_1.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <memory>

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
  virtual ~TextManagerState(){};
  TextManagerState(){};
  TextManagerState(const TextManagerState *state);
  void AddLine(const std::string &line);
  void SetStatus(TextColor color, const std::string &text);
  std::shared_ptr<TextManagerState> Interpolate(const TextManagerState *state,
                                                double alpha);

 private:
#pragma warning(push)
#pragma warning(disable : 4251)
  std::vector<Line> _lines;
#pragma warning(pop)
};

class TESTCOMMON_DLL TextManager {
 public:
  virtual ~TextManager();
  TextManager(IRenderHost *renderHost);
  void SetState(std::shared_ptr<TextManagerState> state);
  void BackBufferChange();
  void BeforeBackBufferChange();
  void BeforeDeviceReset();
  void DrawText();

 private:
#pragma warning(push)
#pragma warning(disable : 4251)
  std::shared_ptr<TextManagerState> _state;
  ComObject<IRenderSettingsManager> _settings;
  ComObject<ID2D1SolidColorBrush> _whiteBrush;
  ComObject<ID2D1SolidColorBrush> _redBrush;
  ComObject<ID2D1SolidColorBrush> _greenBrush;
  ComObject<ID2D1DeviceContext> _d2dContext;
  ComObject<IDWriteFactory1> _dWriteFactory;
  ComObject<IDWriteTextFormat> _textFormat;
  ComObject<ID3D11DeviceContext> _immediateContext;
#pragma warning(pop)
  IRenderHost *_renderHost;
};
}  // namespace Test
}  // namespace MGDF