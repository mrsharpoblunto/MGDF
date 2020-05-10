#pragma once

#include <MGDF/MGDF.hpp>
#include <map>
#include <typeindex>

#include "../common/MGDFSystemComponent.hpp"

namespace MGDF {
namespace core {

/**
this class provides a single repository for MGDF  components based on the
servicelocator pattern
*/
class Components {
 public:
  static Components *InstancePtr() {
    static Components c;
    return &c;
  }

  static Components &Instance() { return *Components::InstancePtr(); }

  virtual ~Components();

  /**
  registers a component type along with an instance of that type. Once a type
  has been registered it cannot be reregistered unless it is first unregistered
  */
  template <class T>
  void RegisterComponent(T *component) {
    std::type_index t = std::type_index(typeid(T));
    if (_components.find(t) == _components.end()) {
      _components.insert(
          std::pair<std::type_index, ISystemComponent *>(t, component));
    }
  }

  /**
  returns a component if it has been previously registered
  */
  template <class T>
  T *Get() {
    std::type_index t = std::type_index(typeid(T));
    auto component = _components.find(t);
    if (component != _components.end()) {
      return static_cast<T *>(component->second);
    }
    return nullptr;
  }

  /**
  removes the class registration from the service locator and also cleans up the
  service It is important that every call to RegisterService is matched with the
  equivalent call to UnregisterService
  */
  template <class T>
  void UnregisterComponent() {
    std::type_index t = std::type_index(typeid(T));
    auto component = _components.find(t);
    if (component != _components.end()) {
      delete component->second;
      _components.erase(component);
    }
  }

  void RegisterComponentErrorHandler(IErrorHandler *errorHandler) {
    for (auto component : _components) {
      component.second->SetComponentErrorHandler(errorHandler);
    }
  }

 private:
  Components();
  std::map<std::type_index, ISystemComponent *> _components;
};

}  // namespace core
}  // namespace MGDF