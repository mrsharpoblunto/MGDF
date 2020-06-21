#include "StdAfx.h"

#include "MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

HRESULT CopyStr(const std::string &out, char *buffer, size_t *length) {
  if (buffer == nullptr) {
    *length = out.size();
    return S_OK;
  }
  return memcpy_s(buffer, *length, out.data(), out.size())
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

HRESULT CopyWStr(const std::wstring &out, wchar_t *buffer, size_t *length) {
  if (buffer == nullptr) {                                  
    *length = out.size();                                   
    return S_OK;                                            
  }                                                         
  return wmemcpy_s(buffer, *length, out.data(), out.size()) 
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

}
}
