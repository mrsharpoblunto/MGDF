#include "stdafx.h"
#include <xercesc/util/PlatformUtils.hpp>

#include "XercesXMLFactoryComponent.hpp"
#include "XercesXMLSchemaCache.hpp"
#include "XercesGameXMLHandler.hpp"
#include "XercesGameStateXMLHandler.hpp"
#include "XercesPreferenceConfigXMLHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

IXMLFactoryComponent *CreateXercesXMLFactoryComponent(HINSTANCE instance,HWND window,bool doValidation)
{
	try {
		return new XercesXMLFactoryComponent(instance,window,doValidation);
	}
	catch (...)
	{
		return NULL;
	}
}

XercesXMLFactoryComponent::XercesXMLFactoryComponent(HINSTANCE instance,HWND window,bool doValidation)
{
	XMLPlatformUtils::Initialize();
	new XercesXmlSchemaCache(doValidation);
}

void XercesXMLFactoryComponent::Dispose()
{
	delete this;
}

XercesXMLFactoryComponent::~XercesXMLFactoryComponent()
{
	delete XercesXmlSchemaCache::InstancePtr();
	XMLPlatformUtils::Terminate();
}

IGameXMLHandler *XercesXMLFactoryComponent::CreateGameXMLHandler() const
{
	return new XercesGameXMLHandler();
}

IGameStateXMLHandler *XercesXMLFactoryComponent::CreateGameStateXMLHandler(std::string game,const Version *version) const
{
	return new XercesGameStateXMLHandler(game,version);
}

IPreferenceConfigXMLHandler *XercesXMLFactoryComponent::CreatePreferenceConfigXMLHandler() const
{
	return new XercesPreferenceConfigXMLHandler();
}

}}}}