#pragma once

#include <Windows.h>

#include <algorithm>
#include <cmath>

namespace MGDF::core {

inline RECT KeepClientAspect(RECT rect, UINT edge, LONG frameWidth,
                             LONG frameHeight, double aspect) {
  if (aspect <= 0 || edge < WMSZ_LEFT || edge > WMSZ_BOTTOMRIGHT) return rect;
  LONG width = (std::max)(1L, rect.right - rect.left - frameWidth);
  LONG height = (std::max)(1L, rect.bottom - rect.top - frameHeight);
  if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM) {
    width = static_cast<LONG>(std::lround(height * aspect));
  } else {
    height = static_cast<LONG>(std::lround(width / aspect));
  }
  width += frameWidth;
  height += frameHeight;
  if (edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT)
    rect.left = rect.right - width;
  else
    rect.right = rect.left + width;
  if (edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
    rect.top = rect.bottom - height;
  else
    rect.bottom = rect.top + height;
  return rect;
}

}  // namespace MGDF::core
