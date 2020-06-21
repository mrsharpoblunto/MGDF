#include "StdAfx.h"

#include "TextManager.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

#define MAX_LINES 100

TextManagerState::TextManagerState(const TextManagerState *state) {
  for (auto line : state->_lines) {
    _lines.push_back(line);
  }
}

void TextManagerState::AddLine(const std::string &line) {
  Line l;
  l.Content = line;

  if (_lines.size() == MAX_LINES) {
    _lines.pop_back();
  }
  _lines.insert(_lines.begin(), l);
}

void TextManagerState::SetStatus(TextColor color, const std::string &text) {
  _lines[0].StatusText = text;
  _lines[0].StatusColor = color;
}

std::shared_ptr<TextManagerState> TextManagerState::Interpolate(
    const TextManagerState *endState, double alpha) {
  (void)alpha;
  // interpolation isn't really possible with this type of gamestate, so just
  // use the most recent.
  return std::shared_ptr<TextManagerState>(new TextManagerState(endState));
}

TextManager::~TextManager() { BeforeDeviceReset(); }

void TextManager::BeforeBackBufferChange() {
  if (_d2dContext) {
    _d2dContext->SetTarget(nullptr);
  }
}

void TextManager::BackBufferChange() {
  if (_d2dContext) {
    _renderHost->SetBackBufferRenderTarget(_d2dContext);
  }
}

void TextManager::BeforeDeviceReset() {
  _whiteBrush.Clear();
  _redBrush.Clear();
  _greenBrush.Clear();
  _d2dContext.Clear();
  _dWriteFactory.Clear();
  _textFormat.Clear();
  _immediateContext.Clear();
}

TextManager::TextManager(IMGDFRenderHost *renderHost) : _renderHost(renderHost) {
  _renderHost->GetRenderSettings(_settings.Assign());
}

void TextManager::SetState(std::shared_ptr<TextManagerState> state) {
  _state = state;
}

void TextManager::DrawText() {
  if (!_immediateContext) {
    _renderHost->GetD3DDevice()->GetImmediateContext(
        _immediateContext.Assign());
    if (FAILED(_renderHost->GetD2DDevice()->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE, _d2dContext.Assign()))) {
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

  if (_state) {
    INT32 starty =
        (_state.get()->_lines.size() * 25 < _settings->GetScreenY())
            ? ((static_cast<UINT32>(_state.get()->_lines.size()) * 25) - 25)
            : (_settings->GetScreenY() - 25);

    _d2dContext->BeginDraw();

    for (auto &line : _state.get()->_lines) {
      std::wstring content;
      content.assign(line.Content.begin(), line.Content.end());

      ComObject<IDWriteTextLayout> textLayout;
      if (FAILED(_dWriteFactory->CreateTextLayout(
              content.c_str(), static_cast<UINT32>(content.size()), _textFormat,
              static_cast<float>(_settings->GetScreenX()),
              static_cast<float>(_settings->GetScreenY()),
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
                _textFormat, static_cast<float>(_settings->GetScreenX()),
                static_cast<float>(_settings->GetScreenY()),
                textLayout.Assign()))) {
          FATALERROR(_renderHost, "Unable to create text layout");
        }

        origin.x = static_cast<float>(_settings->GetScreenX()) - 150.0f;
        origin.y = static_cast<float>(starty);

        _d2dContext->DrawTextLayout(
            origin, textLayout,
            line.StatusColor == TextColor::GREEN ? _greenBrush : _redBrush);
      }

      starty -= 25;
      if (starty <= -25) break;
    }

    _d2dContext->EndDraw();
  }
}

}  // namespace Test
}  // namespace MGDF