#pragma once

#include <dwrite_1.h>

#include <MGDF/ComObject.hpp>
#include <algorithm>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace MGDF {
namespace core {

class TextStream;

struct TextStyle {
  friend class TextStream;

 public:
  virtual ~TextStyle() {}

  static TextStyle Weight(DWRITE_FONT_WEIGHT weight);
  static TextStyle Size(float size);
  static TextStyle Color(float r, float g, float b, float a);
  static TextStyle Pop();

 private:
  TextStyle() : _push(false) {}
  std::shared_ptr<DWRITE_FONT_WEIGHT> _weight;
  std::shared_ptr<float> _size;
  std::shared_ptr<D2D1_COLOR_F> _color;
  bool _push;
};

class TextStream {
 public:
  TextStream(const ComObject<IDWriteFactory1> &factory);
  virtual ~TextStream();
  template <typename T>
  TextStream &operator<<(T const &rhs) {
    _ss << rhs;
    return *this;
  }
  TextStream &operator<<(TextStyle const &style);
  HRESULT GenerateLayout(const ComObject<ID2D1DeviceContext> &context,
                         const ComObject<IDWriteTextFormat> &format,
                         float maxWidth, float maxHeight,
                         ComObject<IDWriteTextLayout> &textLayout);
  void ClearBrushes();
  void ClearText();
  void SetF(int f);
  void Precision(std::streamsize size);

 private:
  HRESULT GetBrush(const ComObject<ID2D1DeviceContext> &context,
                   const D2D1_COLOR_F &color,
                   ComObject<ID2D1SolidColorBrush> &brush);
  void PopStyle(UINT32 offset);

  std::wostringstream _ss;
  std::wstring _prevText;
  ComObject<IDWriteTextFormat> _prevFormat;
  float _prevMaxWidth;
  float _prevMaxHeight;

  ComObject<IDWriteTextLayout> _prevLayout;
  std::vector<std::pair<UINT32, TextStyle>> _openStyles;
  std::vector<std::pair<std::pair<UINT32, UINT32>, TextStyle>> _appliedStyles;
  std::unordered_map<std::string, ComObject<ID2D1SolidColorBrush>> _brushCache;
  ComObject<IDWriteFactory1> _factory;
};  // namespace core

typedef std::pair<std::pair<double, std::pair<size_t, size_t>>, std::string>
    HeatMapPair;

template <typename T, typename U>
void KeyValueHeatMap(
    const std::set<T> &counters,
    std::function<void(const T &input, std::pair<std::string, U> &out)> map,
    TextStream &outputStream) {
  std::vector<HeatMapPair> sorted;

  size_t index = 0;

  // get a list of mappings to names, values, original positions in the list
  for (auto counter : counters) {
    std::pair<std::string, U> kvp;
    map(counter, kvp);
    sorted.push_back(std::make_pair(
        std::make_pair(kvp.second, std::make_pair(index++, 0)), kvp.first));
  }

  // sort in ascending order of the values
  std::sort(sorted.begin(), sorted.end(),
            [](const HeatMapPair &a, const HeatMapPair &b) {
              return a.first.first > b.first.first;
            });

  // now record the new indexes of the sorted positions
  index = 0;
  for (auto &pair : sorted) {
    pair.first.second.second = index++;
  }

  // and re-sort back to the original order using the original positions
  std::sort(sorted.begin(), sorted.end(),
            [](const HeatMapPair &a, const HeatMapPair &b) {
              return a.first.second.first > b.first.second.first;
            });

  // then display the lowest items as closest to white, and the highest as
  // closest to red
  for (const auto &pair : sorted) {
    const float fraction =
        (pair.first.second.second / static_cast<float>(sorted.size()));
    outputStream << "\r\n " << pair.second.c_str() << " : "
                 << TextStyle::Color(1.0f, fraction, fraction, 1.0f)
                 << pair.first.first << TextStyle::Pop();
  }
}

}  // namespace core
}  // namespace MGDF
