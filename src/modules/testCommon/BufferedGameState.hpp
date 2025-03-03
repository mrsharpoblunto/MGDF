#pragma once

#include <memory>
#include <mutex>

template <typename T>
class BufferedGameState {
 public:
  BufferedGameState() : _pendingState(std::make_shared<T>()) {};

  virtual ~BufferedGameState() {};

  std::shared_ptr<T> Pending() { return _pendingState; }

  std::shared_ptr<T> Interpolate(double alpha) {
    std::lock_guard<std::mutex> lock(_stateMutex);
    if (_previousState && _currentState) {
      return std::make_shared<T>(*_previousState, *_currentState, alpha);
    }
    return std::shared_ptr<T>();
  }

  void Flip() {
    std::lock_guard<std::mutex> lock(_stateMutex);
    _previousState = _currentState;
    _currentState = std::make_shared<T>(*_pendingState);
  }

 private:
  std::mutex _stateMutex;
  std::shared_ptr<T> _pendingState;
  std::shared_ptr<T> _currentState;
  std::shared_ptr<T> _previousState;
};
