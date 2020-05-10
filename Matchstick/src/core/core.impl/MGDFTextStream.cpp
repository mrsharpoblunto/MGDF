#include "stdafx.h"

#include "MGDFTextStream.hpp"

#include <functional>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

TextStyle TextStyle::Weight(DWRITE_FONT_WEIGHT weight) {
  TextStyle style;
  style._weight = std::make_shared<DWRITE_FONT_WEIGHT>(weight);
  style._push = true;
  return style;
}

TextStyle TextStyle::Size(float size) {
  TextStyle style;
  style._size = std::make_shared<float>(size);
  style._push = true;
  return style;
}

TextStyle TextStyle::Color(float r, float g, float b, float a) {
  TextStyle style;
  style._color = std::make_shared<D2D1_COLOR_F>();
  style._color->r = r;
  style._color->g = g;
  style._color->b = b;
  style._color->a = a;
  style._push = true;
  return style;
}

TextStyle TextStyle::Pop() {
  TextStyle style;
  style._push = false;
  return style;
}

TextStream::~TextStream() { ClearBrushes(); }

TextStream::TextStream(IDWriteFactory1 *factory)
    : _factory(factory),
      _prevLayout(nullptr),
      _prevFormat(nullptr),
      _prevMaxHeight(-1.0f),
      _prevMaxWidth(-1.0f) {}

TextStream &TextStream::operator<<(TextStyle const &style) {
  auto offset = static_cast<UINT32>(_ss.str().size());
  if (!style._push) {
    PopStyle(offset);
  } else if (style._weight || style._color || style._size) {
    _openStyles.push_back(std::make_pair(offset, style));
  }
  return *this;
}

void TextStream::ClearText() {
  _ss.str(L"");
  _ss.clear();
  _appliedStyles.clear();
  _openStyles.clear();
}

void TextStream::ClearBrushes() {
  for (auto &brush : _brushCache) {
    SAFE_RELEASE(brush.second);
  }
  _brushCache.clear();
}

void TextStream::SetF(int f) { _ss.setf(f); }

void TextStream::Precision(std::streamsize size) { _ss.precision(size); }

HRESULT TextStream::GenerateLayout(ID2D1DeviceContext *context,
                                   IDWriteTextFormat *format, float maxWidth,
                                   float maxHeight,
                                   IDWriteTextLayout **textLayout) {
  std::wstring text = _ss.str();

  // rebuilding layouts is kind of expensive, so lets see if
  // its changed since the last time we generated it
  if (text == _prevText && format == _prevFormat && _prevMaxWidth == maxWidth &&
      _prevMaxHeight == maxHeight && _prevLayout &&
      *textLayout == _prevLayout) {
    return ERROR_SUCCESS;
  }

  UINT32 length = static_cast<UINT32>(text.size());
  while (_openStyles.size()) {
    PopStyle(length);
  }

  if (*textLayout) {
    SAFE_RELEASE(*textLayout);
  }
  HRESULT hr = _factory->CreateTextLayout(text.c_str(), length, format,
                                          maxWidth, maxHeight, textLayout);
  if (FAILED(hr)) {
    return hr;
  }

  for (const auto &style : _appliedStyles) {
    DWRITE_TEXT_RANGE range = {style.first.first,
                               style.first.second - style.first.first};
    if (style.second._size) {
      (*textLayout)->SetFontSize(*style.second._size, range);
    }
    if (style.second._weight) {
      (*textLayout)->SetFontWeight(*style.second._weight, range);
    }
    if (style.second._color) {
      ID2D1SolidColorBrush *brush;
      hr = GetBrush(context, *style.second._color, &brush);
      if (FAILED(hr)) {
        return hr;
      }
      (*textLayout)->SetDrawingEffect(brush, range);
    }
  }

  _prevLayout = *textLayout;
  _prevText = text;
  _prevFormat = format;
  _prevMaxWidth = maxWidth;
  _prevMaxHeight = maxHeight;

  return ERROR_SUCCESS;
}

HRESULT TextStream::GetBrush(ID2D1DeviceContext *context,
                             const D2D1_COLOR_F &color,
                             ID2D1SolidColorBrush **brush) {
  std::ostringstream key;
  key << color.r << '_' << color.g << '_' << color.b << ' ' << color.a;

  auto cached = _brushCache.find(key.str());
  if (cached == _brushCache.end()) {
    HRESULT result = context->CreateSolidColorBrush(color, brush);
    if (FAILED(result)) {
      return result;
    }
    _brushCache.insert(std::make_pair(key.str(), *brush));
  } else {
    *brush = cached->second;
  }
  return ERROR_SUCCESS;
}

void TextStream::PopStyle(UINT32 offset) {
  auto openStyle = _openStyles.back();
  _openStyles.pop_back();
  _appliedStyles.push_back(std::make_pair(
      std::make_pair(openStyle.first, offset), openStyle.second));
}

}  // namespace core
}  // namespace MGDF
