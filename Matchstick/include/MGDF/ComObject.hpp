#pragma once

#include <array>
#include <atomic>
#include <string>

#include "unknwn.h"

namespace MGDF {

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

  explicit ComObject(T *data, bool addRef = false) : _data(data) {
    if (addRef) {
      _data->AddRef();
    }
  }

  // provides a pointer from which to assign this objects data.
  // use this method to allow assignment from Direct3D style
  // methods that take a ** as a parameter.
  T **Assign() {
    Cleanup();
    return &_data;
  }

  template <typename T>
  void AddRawRef(T **raw) {
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

template <typename T>
class ComArray;

template <typename T>
class ComArrayIterator {
 public:
  ComArrayIterator(ComArray<T> *data, size_t index)
      : _data(data), _index(index) {}

  bool operator!=(const ComArrayIterator<T> &other) const {
    return !(*this == other);
  }

  bool operator==(const ComArrayIterator<T> &other) const {
    return _data == other._data && _index == other._index;
  }

  ComArrayIterator &operator++() {
    ++_index;
    return *this;
  }

  ComObject<T> operator*() const {
    if (_index < _data->size()) {
      return _data->at(_index);
    } else {
      return ComObject<T>();
    }
  }

 private:
  ComArray<T> *_data;
  size_t _index;
};

/**
a fixed size array of d3d COM objects. Releasing the objects in the array is
taken care of when the array goes out of scope
*/
template <typename T>
class ComArray {
 public:
  ComArrayIterator<T> begin() { return ComArrayIterator<T>(this, 0); }
  ComArrayIterator<T> end() { return ComArrayIterator<T>(this, _size); }

  ComObject<T> operator[](size_t i) { return ComObject<T>(_data[i], true); }
  ComObject<T> at(size_t i) { return ComObject<T>(_data[i], true); }

  size_t size() const { return _size; }

  ComArray() {
    _data = nullptr;
    _size = 0;
  }

  ComArray(uint32_t size) : _size(size) {
    _data = new T *[_size];
    memset(_data, 0, _size);
  }

  ComArray(ComArray &&object) {
    _data = object._data;
    _size = object._size;
    object._data = nullptr;
    object._size = 0;
  }

  ComArray &operator=(ComArray &&object) {
    _data = object._data;
    _size = object._size;
    object._data = nullptr;
    object._size = 0;
    return *this;
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

template <typename T, typename U>
T ToString(const ComObject<U> &str) {
  T s;
  s.resize(str->GetSize());
  bool result = str->Copy(s.data(), s.size());
  _ASSERTE(result);
  return s;
}

}  // namespace MGDF

namespace std {
// sort and hash this wrapper type just like its pointer equivilant

template <typename T>
struct hash<MGDF::ComObject<T>> {
  std::size_t operator()(const MGDF::ComObject<T> &k) const {
    return hash<T *>()(k.Get());
  }
};

template <typename T>
struct less<MGDF::ComObject<T>> {
  bool operator()(const MGDF::ComObject<T> &left,
                  const MGDF::ComObject<T> &right) const {
    return left.Get() < right.Get();
  }
};

}  // namespace std
