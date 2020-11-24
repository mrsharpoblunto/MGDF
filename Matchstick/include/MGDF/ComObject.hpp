#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <ostream>
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

  T *operator&() = delete;

  operator T *() const { return _data; }

  operator const T *() const { return _data; }

  operator bool() const { return _data != nullptr; }

  T *operator->() const { return _data; }

  // construction/initialization (takes ownership)
  ComObject() : _data(nullptr) {}

  explicit ComObject(T *data, bool addRef = false) : _data(data) {
    if (data && addRef) {
      _data->AddRef();
    }
  }

  // provides a pointer from which to assign this objects data.
  // use this method to allow assignment from Direct3D style
  // methods that take a ** as a parameter.
  T **Assign() {
    Clear();
    return &_data;
  }

  template <typename U>
  void AddRawRef(U **raw) {
    _data->AddRef();
    *raw = _data;
  }

  // copy semantics (shares ownership)
  ComObject(const ComObject &object) : _data(nullptr) { Reassign(object); }

  template <typename U>
  ComObject &operator=(const ComObject<U> &object) {
    Reassign(object);
    return *this;
  }

  ComObject &operator=(const ComObject &object) {
    Reassign(object);
    return *this;
  }

  // move semantics (takes ownership)
  ComObject(ComObject &&object) : _data(object._data) {
    object._data = nullptr;
  }

  ComObject &operator=(ComObject &&object) {
    _data = object;
    object._data = nullptr;
    return *this;
  }

  bool operator==(const ComObject &object) const { return _data == object; }

  // cleanup
  virtual ~ComObject() { Clear(); }

  void Clear() {
    if (_data) {
      _data->Release();
      _data = nullptr;
    }
  }

 private:
  T *_data;

  template <typename U>
  void Reassign(const ComObject<U> &object) {
    if (_data != object) {
      Clear();
      if (object) {
        _data = object.Get();
        _data->AddRef();
      }
    }
  }
};

template <typename T>
class ComArray;

template <typename T>
class ComArrayIterator {
 public:
  ComArrayIterator(const ComArray<T> *data, size_t index)
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

  const ComObject<T> operator*() const {
    if (_index < _data->size()) {
      return _data->at(_index);
    } else {
      return ComObject<T>();
    }
  }

 private:
  const ComArray<T> *_data;
  size_t _index;
};

/**
a fixed size array of d3d COM objects. Releasing the objects in the array is
taken care of when the array goes out of scope
*/
template <typename T>
class ComArray {
 public:
  const ComArrayIterator<T> begin() const {
    return ComArrayIterator<T>(this, 0);
  }
  const ComArrayIterator<T> end() const {
    return ComArrayIterator<T>(this, _size);
  }

  const ComObject<T> operator[](size_t i) const {
    return ComObject<T>(_data[i], true);
  }
  const ComObject<T> at(size_t i) const { return ComObject<T>(_data[i], true); }

  size_t size() const { return _size; }

  ComArray() {
    _data = nullptr;
    _size = 0;
  }

  ComArray(size_t size) : _size(size) {
    _data = std::make_unique<T *[]>(_size);
    memset(_data.get(), 0, _size);
  }

  ComArray(ComArray &&object) {
    _data.swap(object._data);
    _size = object._size;
    object._data.reset();
    object._size = 0;
  }

  ComArray &operator=(ComArray &&object) {
    _data.swap(object._data);
    _size = object._size;
    object._data.reset();
    object._size = 0;
    return *this;
  }

  virtual ~ComArray() {
    for (size_t i = 0; i < _size; ++i) {
      if (_data[i]) {
        _data[i]->Release();
        _data[i] = nullptr;
      }
    }
  }

  T **Data() { return _data.get(); }

 private:
  std::unique_ptr<T *[]> _data;
  size_t _size;
};

template <typename Last>
BOOL IIDMatch(REFIID riid) {
  return (__uuidof(Last) == riid) || (IID_IUnknown == riid);
}

template <typename First, typename Second, typename... Args>
BOOL IIDMatch(REFIID riid) {
  return (__uuidof(First) == riid) || IIDMatch<Second, Args...>(riid);
}

/**
Provides a base implementation of the IUnknown COM interface
*/
template <typename T, typename... Super>
class ComBase : public T {
 public:
  ComBase() : _references(1UL) {}
  virtual ~ComBase() { _ASSERTE(_references == 0UL); }
  ULONG __stdcall AddRef() final { return ++_references; };
  ULONG __stdcall Release() final {
    const ULONG refs = --_references;
    if (refs == 0UL) {
      delete this;
    };
    return refs;
  }
  HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject) final {
    if (!ppvObject) return E_POINTER;
    if (IIDMatch<T, Super...>(riid)) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    }
    return E_NOINTERFACE;
  };

 private:
  std::atomic<ULONG> _references;
};

template <typename T, typename... Args>
ComObject<T> MakeCom(Args &&... args) {
  return ComObject<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename U = T>
ComObject<T> MakeComFromPtr(U *ptr) {
  auto sub = dynamic_cast<T *>(ptr);
  _ASSERTE(sub);
  return ComObject<T>(sub, true);
}

template <auto F>
struct ComString;

template <typename Owner, typename Arg, typename Char,
          HRESULT (Owner::*F)(Arg, Char *, size_t *)>
struct ComString<F> {
 public:
  ComString(Owner *owner, Arg arg) : _owner(owner), _arg(arg) {}

  operator std::basic_string<Char>() const {
    size_t size = 0;
    (_owner->*F)(_arg, nullptr, &size);
    std::basic_string<Char> str(size, '\0');
    (_owner->*F)(_arg, str.data(), &size);
    return str;
  }

  std::ostream &Stream(std::ostream &out) {
    auto str = this->operator std::basic_string<Char>();
    out << str;
    return out;
  }

 private:
  Owner *_owner;
  Arg _arg;
};

template <typename Owner, typename Char, HRESULT (Owner::*F)(Char *, size_t *)>
struct ComString<F> {
 public:
  ComString(Owner *owner) : _owner(owner) {}
  operator std::basic_string<Char>() const {
    size_t size = 0;
    (_owner->*F)(nullptr, &size);
    std::basic_string<Char> str(size, '\0');
    (_owner->*F)(str.data(), &size);
    return str;
  }

  std::ostream &Stream(std::ostream &out) const {
    auto str = this->operator std::basic_string<Char>();
    out << str;
    return out;
  }

 private:
  Owner *_owner;
};

}  // namespace MGDF

namespace std {

template <auto F>
std::ostream &operator<<(std::ostream &out, const MGDF::ComString<F> &c) {
  c.Stream(out);
  return out;
}

// sort and hash this wrapper type just like its pointer equivalent
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
