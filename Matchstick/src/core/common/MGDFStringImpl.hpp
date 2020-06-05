#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <string>

namespace MGDF {
namespace core {

#define COPY_STR(out, buffer, length) \
  if (buffer == nullptr) {            \
    *length = out.size();             \
    return S_OK;                      \
  }                                   \
  return memcpy_s(buffer, *length, out.data(), out.size()) ? E_FAIL : S_OK;

#define COPY_WSTR(out, buffer, length) \
  if (buffer == nullptr) {             \
    *length = out.size();              \
    return S_OK;                       \
  }                                    \
  return wmemcpy_s(buffer, *length, out.data(), out.size()) ? E_FAIL : S_OK;

#define STR(obj, method)            \
  [&]() {                           \
    size_t size = 0;                \
    obj->method(nullptr, &size);    \
    std::string str(size, '\0');    \
    obj->method(str.data(), &size); \
    return str;                     \
  }()

#define WSTR(obj, method)           \
  [&]() {                           \
    size_t size = 0;                \
    obj->method(nullptr, &size);    \
    std::wstring str(size, '\0');   \
    obj->method(str.data(), &size); \
    return str;                     \
  }()
}  // namespace core
}  // namespace MGDF
