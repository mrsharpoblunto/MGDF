#include "StdAfx.h"

#include "MGDFWindowIcon.hpp"

#include <wincodec.h>

#include <MGDF/ComObject.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

HICON LoadIconFromImage(const std::filesystem::path &path, UINT size) {
  if (!size || !std::filesystem::exists(path)) return nullptr;

  ComObject<IWICImagingFactory> factory;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(factory.Assign())))) {
    return nullptr;
  }

  ComObject<IWICBitmapDecoder> decoder;
  if (FAILED(factory->CreateDecoderFromFilename(
          path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
          decoder.Assign()))) {
    return nullptr;
  }

  ComObject<IWICBitmapFrameDecode> frame;
  UINT sourceWidth = 0, sourceHeight = 0;
  if (FAILED(decoder->GetFrame(0, frame.Assign())) ||
      FAILED(frame->GetSize(&sourceWidth, &sourceHeight)) || !sourceWidth ||
      !sourceHeight) {
    return nullptr;
  }

  const double scale = (std::min)(static_cast<double>(size) / sourceWidth,
                                static_cast<double>(size) / sourceHeight);
  const UINT width = std::clamp<UINT>(
      static_cast<UINT>(std::lround(sourceWidth * scale)), 1, size);
  const UINT height = std::clamp<UINT>(
      static_cast<UINT>(std::lround(sourceHeight * scale)), 1, size);

  ComObject<IWICBitmapScaler> scaler;
  ComObject<IWICFormatConverter> converter;
  if (FAILED(factory->CreateBitmapScaler(scaler.Assign())) ||
      FAILED(scaler->Initialize(frame, width, height,
                                WICBitmapInterpolationModeFant)) ||
      FAILED(factory->CreateFormatConverter(converter.Assign())) ||
      FAILED(converter->Initialize(scaler, GUID_WICPixelFormat32bppBGRA,
                                   WICBitmapDitherTypeNone, nullptr, 0.0,
                                   WICBitmapPaletteTypeCustom))) {
    return nullptr;
  }

  const UINT stride = size * 4;
  const UINT offset = ((size - height) / 2) * stride + ((size - width) / 2) * 4;
  std::vector<BYTE> pixels(static_cast<size_t>(stride) * size, 0);
  if (FAILED(converter->CopyPixels(
          nullptr, stride, static_cast<UINT>(pixels.size() - offset),
          pixels.data() + offset))) {
    return nullptr;
  }

  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = static_cast<LONG>(size);
  info.bmiHeader.biHeight = -static_cast<LONG>(size);
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;

  void *bits = nullptr;
  HBITMAP color = ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits,
                                     nullptr, 0);
  if (!color) return nullptr;
  ::memcpy(bits, pixels.data(), pixels.size());

  // the alpha channel carries the transparency, so the mask stays empty
  HBITMAP mask = ::CreateBitmap(static_cast<int>(size),
                                static_cast<int>(size), 1, 1, nullptr);
  ICONINFO iconInfo{
      .fIcon = TRUE,
      .xHotspot = 0,
      .yHotspot = 0,
      .hbmMask = mask,
      .hbmColor = color,
  };
  HICON icon = mask ? ::CreateIconIndirect(&iconInfo) : nullptr;
  if (mask) ::DeleteObject(mask);
  ::DeleteObject(color);
  return icon;
}

}  // namespace core
}  // namespace MGDF
