#pragma once

#include <MGDF/MGDF.h>
#include <d2d1_1.h>
#include <dwrite_1.h>

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
  // the host draws no overlay; the test module shows it on ALT+F12
  void ToggleOverlay() { _overlayShown = !_overlayShown; }
  bool OverlayShown() const { return _overlayShown; }

  static constexpr UINT32 LINE_HEIGHT = 25;
  static constexpr float SCROLLBAR_WIDTH = 12.0f;

  // scrolls the view by the given number of lines (positive scrolls
  // back into older output, negative towards the newest)
  void Scroll(INT32 lines, UINT32 screenHeight);
  // scrolls the view so the scrollbar thumb centers on the given
  // vertical mouse position
  void SetScroll(float mouseY, UINT32 screenHeight);

  // when set, all test output lines and status changes are also
  // forwarded to this sink (e.g. to mirror them into the MGDF log)
#pragma warning(push)
#pragma warning(disable : 4251)
  static std::function<void(const std::string &)> LogSink;
#pragma warning(pop)

 private:
  size_t MaxScrollOffset(UINT32 screenHeight) const;
  float ThumbHeight(UINT32 screenHeight) const;

#pragma warning(push)
#pragma warning(disable : 4251)
  std::vector<Line> _lines;
#pragma warning(pop)
  // how many lines back from the newest the view is scrolled
  // (0 = pinned to the latest output)
  size_t _scrollOffset = 0;
  bool _overlayShown = false;
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
  void DrawOverlay();

#pragma warning(push)
#pragma warning(disable : 4251)
  TextManagerState _state;
  ComObject<IMGDFRenderSettingsManager> _settings;
  ComObject<IMGDFDebug> _debug;
  ComObject<ID2D1SolidColorBrush> _whiteBrush;
  ComObject<ID2D1SolidColorBrush> _redBrush;
  ComObject<ID2D1SolidColorBrush> _greenBrush;
  ComObject<ID2D1SolidColorBrush> _overlayBackgroundBrush;
  ComObject<ID2D1Factory1> _d2dFactory;
  ComObject<ID2D1Device> _d2dDevice;
  ComObject<ID2D1DeviceContext> _d2dContext;
  ComObject<ID2D1Bitmap1> _targetBitmap;
  ComObject<IDWriteFactory1> _dWriteFactory;
  ComObject<IDWriteTextFormat> _textFormat;
  ComObject<IDWriteTextFormat> _overlayTextFormat;
#pragma warning(pop)
  IMGDFRenderHost *_renderHost;
};
}  // namespace Test
}  // namespace MGDF