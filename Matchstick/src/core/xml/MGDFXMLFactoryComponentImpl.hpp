#include "../common/MGDFComponents.hpp"

#ifdef USE_XERCES_XML
#include "xerces/XercesXMLFactoryComponent.hpp"
#endif


namespace MGDF { namespace core { namespace xml {

	//provide a generic name for the rest of the system so that
	//there is no explicit instansiation of a particular xml
	//implementation
	#ifdef USE_XERCES_XML
	#define CreateXMLFactoryComponentImpl xercesImpl::CreateXercesXMLFactoryComponent
	#endif

}}}