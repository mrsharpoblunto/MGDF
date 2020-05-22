#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <string>

namespace MGDF {
namespace core {

template <typename T, typename U>
class StringImplBase : public ComBase<U> {
 public:
  StringImplBase(const T* data) : _data(data) {}
  virtual ~StringImplBase() {}

  virtual size_t GetSize() const final { return _data.size(); }

  virtual bool Copy(T* buffer, size_t size) const final {
    // allow one more character if the user wants to copy the final null
    // terminator
    if (size > _data.size() + 1) return false;
    return memcpy_s(buffer, size, _data.c_str(), size) == 0;
  }

 private:
  std::basic_string<T> _data;
};

typedef StringImplBase<char, IString> StringImpl;
typedef StringImplBase<wchar_t, IWString> WStringImpl;

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
