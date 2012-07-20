#include "../common/MGDFComponents.hpp"

#ifdef USE_JSONCPP_STORAGE
#include "jsoncpp/JsonCppStorageFactoryComponent.hpp"
#endif


namespace MGDF { namespace core { namespace xml {

	//provide a generic name for the rest of the system so that
	//there is no explicit instansiation of a particular xml
	//implementation
	#ifdef USE_JSONCPP_STORAGE
	#define CreateStorageFactoryComponentImpl jsoncppImpl::CreateJsonCppStorageFactoryComponent
	#endif

}}}