#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <functional>
#include <string>

namespace MGDF {
namespace core {

HRESULT CopyStr(const std::string &out, char *buffer, size_t *length);
HRESULT CopyWStr(const std::wstring &out, wchar_t *buffer, size_t *length);

template <typename T>
std::basic_string<T> ToStr(std::function<void(T *, size_t *)> handler) {
  size_t size = 0;
  handler(nullptr, &size);
  std::basic_string<T> str(size, '\0');
  handler(str.data(), &size);
  return str;
}

#define STR(obj, method) ToStr<char>([&](auto buffer,auto size) { obj->method(buffer, size); })
#define WSTR(obj, method) ToStr<wchar_t>([&](auto buffer,auto size) { obj->method(buffer, size); })

}  // namespace core
}  // namespace MGDF
