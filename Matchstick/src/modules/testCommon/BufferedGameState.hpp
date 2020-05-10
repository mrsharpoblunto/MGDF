#pragma once

#include <memory>
#include <mutex>

template <typename T>
class BufferedGameState {
 public:
  BufferedGameState() {
    _pendingState = new T();
    _currentState = nullptr;
    _previousState = nullptr;
  };

  virtual ~BufferedGameState() {
    if (_pendingState) delete _pendingState;
    if (_currentState) delete _currentState;
    if (_previousState) delete _previousState;
  };

  T *Pending() { return _pendingState; }

  std::shared_ptr<T> Interpolate(double alpha) {
    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_previousState != nullptr && _currentState != nullptr) {
      return std::shared_ptr<T>(
          _previousState->Interpolate(_currentState, alpha));
    }
    return std::shared_ptr<T>();
  }

  void Flip() {
    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_previousState) delete _previousState;
    _previousState = _currentState;
    _currentState = new T(_pendingState);
  }

 private:
  std::mutex _stateMutex;
  T *_pendingState;
  T *_currentState;
  T *_previousState;
};
