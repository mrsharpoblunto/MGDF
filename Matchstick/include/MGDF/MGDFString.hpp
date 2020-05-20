#pragma once

namespace MGDF {

MIDL_INTERFACE("70EEA418-9BF0-458D-BD62-93E4ABFACB2C")
IString : public IUnknown {
 public:
  virtual size_t STDMETHODCALLTYPE GetSize() const = 0;
  virtual bool STDMETHODCALLTYPE Copy(char *buffer, size_t size) const = 0;
};

MIDL_INTERFACE("BBD63ECD-8D9C-4AA2-9E37-40B8F0AE8D7F")
IWString : public IUnknown {
 public:
  virtual size_t STDMETHODCALLTYPE GetSize() const = 0;
  virtual bool STDMETHODCALLTYPE Copy(wchar_t * buffer, size_t size) const = 0;
};

}  // namespace MGDF
