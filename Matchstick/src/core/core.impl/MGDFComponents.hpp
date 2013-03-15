#pragma once

#include <MGDF/MGDF.hpp>
#include <map>
#include <typeinfo>
#include "../common/MGDFSystemComponent.hpp"

namespace MGDF
{
namespace core
{

/**
this class provides a single repository for MGDF system components based on the servicelocator pattern
*/
class Components
{
public:
	static Components *InstancePtr() {
		static Components c;
		return &c;
	}

	static Components &Instance() {
		return *Components::InstancePtr();
	}

	virtual ~Components();

	/**
	registers a component type along with an instance of that type. Once a type has been registered it cannot be reregistered
	unless it is first unregistered
	*/
	template<class T>
	void RegisterComponent( T *component ) {
		std::string t = typeid( T ).name();
		if ( _components.find( t ) == _components.end() ) {
			_components.insert( std::pair<std::string,void *>( t, component) );
		}
	}

	/**
	returns a component if it has been previously registered
	*/
	template<class T>
	T *Get() {
		auto component = _components.find( typeid( T ).name() );
		if ( component != _components.end() ) {
			return static_cast <T *>(component->second);
		}
		return nullptr;
	}

	/**
	removes the class registration from the service locator and also cleans up the service
	It is important that every call to RegisterService is matched with the equivalent call to UnregisterService
	*/
	template<class T>
	void UnregisterComponent() {
		auto component = _components.find( typeid( T ).name() );
		if ( component != _components.end() ) {
			delete static_cast <ISystemComponent *>(component->second);
			_components.erase( component );
		}
	}

	void RegisterComponentErrorHandler( IErrorHandler *errorHandler ) {
		for ( auto component : _components ) {
			static_cast <ISystemComponent *>(component.second)->SetComponentErrorHandler( errorHandler );
		}
	}

private:
	Components();
	std::map<std::string, void *> _components;
};

}
}