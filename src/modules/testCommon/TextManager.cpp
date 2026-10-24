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

#define MAX_LINES 100

TextManagerState::TextManagerState(const TextManagerState &startState,
                                   const TextManagerState &endState,
                                   double alpha)
    : _lines(endState._lines),
      _scrollOffset(endState._scrollOffset),
      _overlayShown(endState._overlayShown) {
  std::ignore = alpha;
  std::ignore = startState;
}

TextManagerState::TextManagerState(const TextManagerState &state)
    : _lines(state._lines),
      _scrollOffset(state._scrollOffset),
      _overlayShown(state._overlayShown) {}

std::function<void(const std::string &)> TextManagerState::LogSink;

void TextManagerState::AddLine(const std::string &line) {
  Line l;
  l.Content = line;

  if (_lines.size() == MAX_LINES) {
    _lines.pop_back();
  }
  _lines.insert(_lines.begin(), l);
  // if the view is scrolled back into older output, keep it anchored
  // there as new lines arrive
  if (_scrollOffset > 0 && _scrollOffset < _lines.size() - 1) {
    ++_scrollOffset;
  }
  if (LogSink && !line.empty()) {
    LogSink(line);
  }
}

size_t TextManagerState::MaxScrollOffset(UINT32 screenHeight) const {
  const size_t visible = (std::max)(1u, screenHeight / LINE_HEIGHT);
  return _lines.size() > visible ? _lines.size() - visible : 0;
}

float TextManagerState::ThumbHeight(UINT32 screenHeight) const {
  const size_t visible = (std::max)(1u, screenHeight / LINE_HEIGHT);
  if (_lines.size() <= visible) {
    return static_cast<float>(screenHeight);
  }
  return (std::max)(30.0f, static_cast<float>(screenHeight) *
                             static_cast<float>(visible) /
                             static_cast<float>(_lines.size()));
}

void TextManagerState::Scroll(INT32 lines, UINT32 screenHeight) {
  const size_t maxOffset = MaxScrollOffset(screenHeight);
  const INT64 offset = static_cast<INT64>(_scrollOffset) + lines;
  _scrollOffset = static_cast<size_t>(
      std::clamp<INT64>(offset, 0, static_cast<INT64>(maxOffset)));
}

void TextManagerState::SetScroll(float mouseY, UINT32 screenHeight) {
  const size_t maxOffset = MaxScrollOffset(screenHeight);
  if (!maxOffset) {
    _scrollOffset = 0;
    return;
  }
  const float thumbHeight = ThumbHeight(screenHeight);
  const float track = static_cast<float>(screenHeight) - thumbHeight;
  const float normalized =
      track > 0.0f
          ? std::clamp((mouseY - thumbHeight * 0.5f) / track, 0.0f, 1.0f)
          : 1.0f;
  // normalized 0 = oldest output at the top of the view,
  // 1 = pinned to the newest output at the bottom
  _scrollOffset =
      maxOffset -
      static_cast<size_t>(std::lround(normalized * maxOffset));
}

void TextManagerState::SetStatus(TextColor color, const std::string &text) {
  _lines[0].StatusText = text;
  _lines[0].StatusColor = color;
  if (LogSink) {
    LogSink(_lines[0].Content + " " + text);
  }
}

TextManager::~TextManager() { BeforeDeviceReset(); }

void TextManager::BeforeBackBufferChange() {
  if (_d2dContext) {
    _d2dContext->SetTarget(nullptr);
  }
  _targetBitmap.Clear();
}

void TextManager::BackBufferChange() {
  if (!_d2dContext) return;
  BeforeBackBufferChange();

  ComObject<ID3D11Texture2D> backBuffer;
  auto d3d11Host =
      MakeComFromPtr<IMGDFRenderHost>(_renderHost).As<IMGDFD3D11RenderHost>();
  if (!d3d11Host) {
    FATALERROR(_renderHost, "D3D11 render host is not available");
    return;
  }
  d3d11Host->GetBackBuffer(backBuffer.Assign());
  D3D11_TEXTURE2D_DESC desc;
  backBuffer->GetDesc(&desc);

  ComObject<IDXGISurface> surface;
  if (FAILED(backBuffer->QueryInterface<IDXGISurface>(surface.Assign()))) {
    FATALERROR(_renderHost, "Unable to acquire IDXGISurface from backbuffer");
    return;
  }

  D2D1_BITMAP_PROPERTIES1 properties{};
  properties.bitmapOptions =
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
  properties.pixelFormat = {desc.Format, D2D1_ALPHA_MODE_IGNORE};
  properties.dpiX = properties.dpiY = 96.0f;
  if (FAILED(_d2dContext->CreateBitmapFromDxgiSurface(
          surface, properties, _targetBitmap.Assign()))) {
    FATALERROR(_renderHost, "Unable to create backbuffer target bitmap");
    return;
  }
  _d2dContext->SetTarget(_targetBitmap);
}

void TextManager::BeforeDeviceReset() {
  BeforeBackBufferChange();
  _whiteBrush.Clear();
  _redBrush.Clear();
  _greenBrush.Clear();
  _overlayBackgroundBrush.Clear();
  _d2dContext.Clear();
  _d2dDevice.Clear();
  _d2dFactory.Clear();
  _textFormat.Clear();
  _overlayTextFormat.Clear();
  _dWriteFactory.Clear();
}

TextManager::TextManager(IMGDFRenderHost *renderHost)
    : _renderHost(renderHost) {
  _renderHost->GetRenderSettings(_settings.Assign());
  _renderHost->GetDebug(_debug.Assign());
}

void TextManager::SetState(TextManagerState &state) {
  _state = std::move(state);
}

void TextManager::DrawText() {
  if (!_d2dContext) {
    ComObject<ID3D11Device> d3dDevice;
    auto d3d11Host =
        MakeComFromPtr<IMGDFRenderHost>(_renderHost).As<IMGDFD3D11Host>();
    if (!d3d11Host) {
      FATALERROR(_renderHost, "D3D11 host is not available");
      return;
    }
    d3d11Host->GetD3D11Device(d3dDevice.Assign());
    ComObject<IDXGIDevice> dxgiDevice;
    if (FAILED(d3dDevice->QueryInterface<IDXGIDevice>(dxgiDevice.Assign()))) {
      FATALERROR(_renderHost, "Unable to acquire IDXGIDevice from ID3D11Device");
      return;
    }

    const D2D1_FACTORY_OPTIONS options{
#if defined(_DEBUG)
        .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
#else
        .debugLevel = D2D1_DEBUG_LEVEL_NONE
#endif
    };
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options,
                                _d2dFactory.Assign()))) {
      FATALERROR(_renderHost, "Unable to create ID2D1Factory1");
      return;
    }
    if (FAILED(_d2dFactory->CreateDevice(dxgiDevice, _d2dDevice.Assign()))) {
      FATALERROR(_renderHost, "Unable to create ID2D1Device");
      return;
    }
    if (FAILED(_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                             _d2dContext.Assign()))) {
      FATALERROR(_renderHost, "Unable to create ID2D1DeviceContext");
    }
    BackBufferChange();

    if (FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1),
            reinterpret_cast<IUnknown **>(_dWriteFactory.Assign())))) {
      FATALERROR(_renderHost, "Unable to create IDWriteFactory");
    }

    ComObject<IDWriteFontCollection> fontCollection;
    if (FAILED(
            _dWriteFactory->GetSystemFontCollection(fontCollection.Assign()))) {
      FATALERROR(_renderHost, "Unable to get  font collection");
    }

    if (FAILED(_dWriteFactory->CreateTextFormat(
            L"Arial", fontCollection, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 22, L"",
            _textFormat.Assign()))) {
      FATALERROR(_renderHost, "Unable to create text format");
    }

    if (FAILED(_dWriteFactory->CreateTextFormat(
            L"Arial", fontCollection, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14, L"",
            _overlayTextFormat.Assign()))) {
      FATALERROR(_renderHost, "Unable to create overlay text format");
    }

    const D2D1_COLOR_F background{0.05f, 0.05f, 0.05f, 0.85f};
    if (FAILED(_d2dContext->CreateSolidColorBrush(
            background, _overlayBackgroundBrush.Assign()))) {
      FATALERROR(_renderHost, "Unable to create overlay background brush");
    }

    D2D1_COLOR_F color;
    color.a = color.r = color.g = color.b = 1.0f;
    if (FAILED(
            _d2dContext->CreateSolidColorBrush(color, _whiteBrush.Assign()))) {
      FATALERROR(_renderHost, "Unable to create white color brush");
    }

    color.g = color.b = 0.5f;
    if (FAILED(_d2dContext->CreateSolidColorBrush(color, _redBrush.Assign()))) {
      FATALERROR(_renderHost, "Unable to create red color brush");
    }

    color.g = 1.0f;
    color.r = 0.5f;
    if (FAILED(
            _d2dContext->CreateSolidColorBrush(color, _greenBrush.Assign()))) {
      FATALERROR(_renderHost, "Unable to create green color brush");
    }
  }

  const UINT32 screenX = _settings->GetScreenX();
  const UINT32 screenY = _settings->GetScreenY();
  const size_t offset =
      _state._lines.empty()
          ? 0
          : (std::min)(_state._scrollOffset, _state._lines.size() - 1);
  const size_t remaining = _state._lines.size() - offset;

  INT32 starty =
      (remaining * TextManagerState::LINE_HEIGHT < screenY)
          ? ((static_cast<UINT32>(remaining) * TextManagerState::LINE_HEIGHT) -
             TextManagerState::LINE_HEIGHT)
          : (screenY - TextManagerState::LINE_HEIGHT);

  _d2dContext->BeginDraw();

  for (auto it = _state._lines.begin() + offset; it != _state._lines.end();
       ++it) {
    auto &line = *it;
    std::wstring content;
    content.assign(line.Content.begin(), line.Content.end());

    ComObject<IDWriteTextLayout> textLayout;
    if (FAILED(_dWriteFactory->CreateTextLayout(
            content.c_str(), static_cast<UINT32>(content.size()), _textFormat,
            static_cast<float>(screenX), static_cast<float>(screenY),
            textLayout.Assign()))) {
      FATALERROR(_renderHost, "Unable to create text layout");
    }

    D2D_POINT_2F origin;
    origin.x = 0;
    origin.y = static_cast<float>(starty);

    _d2dContext->DrawTextLayout(origin, textLayout, _whiteBrush);

    if (line.StatusText != "") {
      std::wstring statusText;
      statusText.assign(line.StatusText.begin(), line.StatusText.end());

      if (FAILED(_dWriteFactory->CreateTextLayout(
              statusText.c_str(), static_cast<UINT32>(statusText.size()),
              _textFormat, static_cast<float>(screenX),
              static_cast<float>(screenY), textLayout.Assign()))) {
        FATALERROR(_renderHost, "Unable to create text layout");
      }

      origin.x = static_cast<float>(screenX) - 150.0f;
      origin.y = static_cast<float>(starty);

      _d2dContext->DrawTextLayout(
          origin, textLayout,
          line.StatusColor == TextColor::GREEN ? _greenBrush : _redBrush);
    }

    starty -= TextManagerState::LINE_HEIGHT;
    if (starty <= -static_cast<INT32>(TextManagerState::LINE_HEIGHT)) break;
  }

  // draw a scrollbar on the right when the output overflows the screen
  const size_t maxOffset = _state.MaxScrollOffset(screenY);
  if (maxOffset > 0) {
    const float thumbHeight = _state.ThumbHeight(screenY);
    const float track = static_cast<float>(screenY) - thumbHeight;
    const float fromTop =
        static_cast<float>(maxOffset - (std::min)(offset, maxOffset)) /
        static_cast<float>(maxOffset);
    const float thumbTop = fromTop * track;
    const float left =
        static_cast<float>(screenX) - TextManagerState::SCROLLBAR_WIDTH;

    const D2D1_RECT_F trackRect{left, 0.0f, static_cast<float>(screenX),
                                static_cast<float>(screenY)};
    _whiteBrush->SetOpacity(0.15f);
    _d2dContext->FillRectangle(trackRect, _whiteBrush);

    const D2D1_RECT_F thumbRect{left, thumbTop, static_cast<float>(screenX),
                                thumbTop + thumbHeight};
    _whiteBrush->SetOpacity(0.6f);
    _d2dContext->FillRectangle(thumbRect, _whiteBrush);
    _whiteBrush->SetOpacity(1.0f);
  }

  DrawOverlay();
  _d2dContext->EndDraw();
}

void TextManager::DrawOverlay() {
  if (!_state.OverlayShown()) return;

  ComObject<IMGDFDebugOverlaySnapshot> snapshot;
  if (FAILED(_debug->GetOverlaySnapshot(snapshot.Assign()))) {
    FATALERROR(_renderHost, "Unable to get overlay snapshot");
    return;
  }
  const auto data = snapshot->GetData();
  std::ostringstream text;
  text << "MGDF Version: " << data->Version
       << "\nMGDF Interface version: " << data->InterfaceVersion;
  if (data->HasTimings) {
    const auto fps = [&text](const char *name, double time) {
      text << "\n " << name << " : ";
      if (time > 0) {
        text << std::fixed << std::setprecision(0) << 1 / time;
      } else {
        text << "N/A";
      }
    };
    const auto timing = [&text](const char *name, double time) {
      text << "\n " << name << " : " << std::fixed << std::setprecision(2)
           << time * 1000 << " ms";
    };
    text << "\n\nRender Thread";
    fps("FPS", data->RenderTime.Average);
    timing("Render CPU", data->ActiveRenderTime.Average);
    timing("Idle CPU",
           data->RenderTime.Average - data->ActiveRenderTime.Average);
    text << "\n\nSim Thread";
    fps("Expected FPS", data->ExpectedSimTime);
    fps("Actual FPS", data->SimTime.Average);
    timing("Input CPU", data->SimInputTime.Average);
    timing("Audio CPU", data->SimAudioTime.Average);
    timing("Other CPU", data->ActiveSimTime.Average);
    timing("Idle CPU",
           data->SimTime.Average - data->ActiveSimTime.Average -
               data->SimInputTime.Average - data->SimAudioTime.Average);
  }
  if (data->CounterCount) text << "\n\nPerformance Counters";
  for (UINT64 i = 0; i < data->CounterCount; ++i) {
    const auto &counter = data->Counters[i];
    text << "\n " << counter.Name << (counter.GPU ? " GPU : " : " CPU : ")
         << std::fixed << std::setprecision(2)
         << counter.Timing.Average * 1000 << " ms";
  }
  std::string section;
  for (UINT64 i = 0; i < data->EntryCount; ++i) {
    const auto &entry = data->Entries[i];
    if (i == 0 || section != entry.Section) {
      section = entry.Section;
      text << "\n\n" << section;
    }
    text << "\n " << entry.Key << " : " << entry.Value;
  }

  const std::string content = text.str();
  const int length = MultiByteToWideChar(CP_UTF8, 0, content.data(),
                                        static_cast<int>(content.size()),
                                        nullptr, 0);
  std::wstring wideContent(length, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, content.data(),
                      static_cast<int>(content.size()), wideContent.data(),
                      length);
  ComObject<IDWriteTextLayout> layout;
  if (FAILED(_dWriteFactory->CreateTextLayout(
          wideContent.c_str(), static_cast<UINT32>(wideContent.size()),
          _overlayTextFormat, static_cast<float>(_settings->GetScreenX()),
          static_cast<float>(_settings->GetScreenY()), layout.Assign()))) {
    FATALERROR(_renderHost, "Unable to create overlay text layout");
    return;
  }
  DWRITE_TEXT_METRICS metrics{};
  if (FAILED(layout->GetMetrics(&metrics))) {
    FATALERROR(_renderHost, "Unable to get overlay text metrics");
    return;
  }
  const D2D1_RECT_F rect{5.0f, 5.0f, metrics.width + 15.0f,
                        metrics.height + 15.0f};
  _d2dContext->FillRectangle(rect, _overlayBackgroundBrush);
  _d2dContext->DrawTextLayout({10.0f, 10.0f}, layout, _whiteBrush);
}

}  // namespace Test
}  // namespace MGDF