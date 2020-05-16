#pragma once

#include <array>
#include <atomic>

#include "unknwn.h"

namespace MGDF {
namespace core {

/**
a fixed size array of COM objects. Releasing the objects in the array is
taken care of when the array goes out of scope
*/
template <typename T>
class ComArray {
 public:
  ComArray(uint32_t size) : _size(size) {
    _data = new T *[_size];
    memset(_data, 0, _size);
  }

  virtual ~ComArray() {
    for (uint32_t i = 0; i < _size; ++i) {
      if (_data[i]) {
        _data[i]->Release();
        _data[i] = nullptr;
      }
    }
    if (_data) {
      delete _data;
      _data = nullptr;
    }
  }

  T *operator[](uint32_t index) const {
    _ASSERTE(index < _size);
    return _data[index];
  }

  T **Data() { return _data; }

 private:
  T **_data;
  uint32_t _size;
};

/**
Provides a base implementation of the IUnknown COM interface
*/
template <typename T>
class ComBase : public T {
 public:
  ComBase() : _references(1UL) {}
  virtual ~ComBase() { _ASSERTE(_references == 0UL); }
  ULONG AddRef() override { return ++_references; };
  ULONG Release() override {
    ULONG refs = --_references;
    if (refs == 0UL) {
      delete this;
    };
    return refs;
  }
  HRESULT QueryInterface(REFIID riid, void **ppvObject) {
    if (!ppvObject) return E_POINTER;
    if (riid == IID_IUnknown || riid == __uuidof(T)) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    }
    return E_NOINTERFACE;
  };

 private:
  std::atomic<ULONG> _references;
};

/**
takes ownership of a COM object and ensures that it is released when
it is no longer used
*/
template <typename T>
class ComObject {
 public:
  template <typename U>
  ComObject<U> As() {
    if (!_data) {
      return ComObject<U>();
    } else {
      U *result;
      if (FAILED(_data->QueryInterface(__uuidof(U), (void **)&result))) {
        return ComObject<U>();
      }
      return ComObject<U>(result);
    }
  }

  T *Get() const { return _data; }

  T *const *AsArray() const { return &_data; }

  T *operator&() const { return _data; }

  operator T *() const { return _data; }

  operator const T *() const { return _data; }

  operator bool() const { return _data != nullptr; }

  T *operator->() const { return _data; }

  // construction/initialization (takes ownership)
  ComObject() : _data(nullptr) {}

  explicit ComObject(T *data) : _data(data) {}

  // provides a pointer from which to assign this objects data.
  // use this method to allow assignment from Direct3D style
  // methods that take a ** as a parameter.
  T **Assign() {
    Cleanup();
    return &_data;
  }

  template <typename T>
  void AssignToRaw(T **raw) {
    _data->AddRef();
    static_cast<T *>(*raw) = _data;
  }

  ComObject &operator=(T *data) {
    if (_data != data) {
      Cleanup();
      _data = data;
    }
    return *this;
  }

  // copy semantics (shares ownership)
  ComObject(const ComObject &object) : _data(nullptr) { Reassign(object); }

  ComObject &operator=(const ComObject &object) {
    Reassign(object);
    return *this;
  }

  // move semantics (takes ownership)
  ComObject(ComObject &&object) : _data(object._data) {
    object._data = nullptr;
  }

  ComObject &operator=(ComObject &&object) {
    _data = object._data;
    object._data = nullptr;
    return *this;
  }

  bool operator==(const ComObject &object) const {
    return _data == object._data;
  }

  // cleanup
  virtual ~ComObject() { Cleanup(); }

 private:
  T *_data;

  void Reassign(const ComObject &object) {
    if (_data != object._data) {
      Cleanup();
      if (object._data) {
        _data = object._data;
        _data->AddRef();
      }
    }
  }

  void Cleanup() {
    if (_data) {
      _data->Release();
      _data = nullptr;
    }
  }
};

}  // namespace core
}  // namespace MGDF