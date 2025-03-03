#include "StdAfx.h"

#include "MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

HRESULT StringWriter::Write(const std::string &out, char *buffer,
                            size_t *length) {
  const auto inputLength = *length;
  *length = out.size();

  if (buffer == nullptr) {
    return S_OK;
  }
  return memcpy_s(buffer, inputLength, out.data(), out.size())
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

HRESULT StringWriter::Write(const std::wstring &out, wchar_t *buffer,
                            size_t *length) {
  const auto inputLength = *length;
  *length = out.size();

  if (buffer == nullptr) {
    return S_OK;
  }
  return memcpy_s(buffer, inputLength * sizeof(wchar_t), out.data(),
                  out.size() * sizeof(wchar_t))
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

}  // namespace core
}  // namespace MGDF
