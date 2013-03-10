#pragma once

#include <MGDF/MGDF.hpp>
#include <boost/unordered_map.hpp>
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
			_components[t] = ( void * ) component;
		}
	}

	/**
	returns a component if it has been previously registered
	*/
	template<class T>
	T *Get() {
		std::string t = typeid( T ).name();
		if ( _components.find( t ) != _components.end() ) {
			return ( T * ) _components[t];
		}
		return nullptr;
	}

	/**
	removes the class registration from the service locator and also cleans up the service
	It is important that every call to RegisterService is matched with the equivalent call to UnregisterService
	*/
	template<class T>
	void UnregisterComponent() {
		std::string t = typeid( T ).name();
		boost::unordered_map<std::string, void *>::iterator iter = _components.find( t );
		if ( iter != _components.end() ) {
			T *temp = ( T * ) _components[t];
			_components.erase( iter );
			SAFE_DELETE( temp );
		}
	}

	void RegisterComponentErrorHandler( IErrorHandler *errorHandler ) {
		for ( boost::unordered_map<std::string, void *>::iterator iter = _components.begin(); iter != _components.end(); ++iter ) {
			( ( ISystemComponent * ) iter->second )->SetComponentErrorHandler( errorHandler );
		}
	}

private:
	Components();
	boost::unordered_map<std::string, void *> _components;
};

}
}