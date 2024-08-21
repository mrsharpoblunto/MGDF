#pragma once

#include <string>

namespace MGDF {
namespace core {

struct StringWriter {
  static HRESULT Write(const std::string &out, char *buffer, size_t *length);
  static HRESULT Write(const std::wstring &out, wchar_t *buffer,
                       size_t *length);
};

}  // namespace core
}  // namespace MGDF
