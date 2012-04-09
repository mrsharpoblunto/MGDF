#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFXMLFactoryComponent.hpp"

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

class XercesXMLFactoryComponent: public DisposeImpl<IXMLFactoryComponent>
{
public:
	XercesXMLFactoryComponent(HINSTANCE instance,HWND window);
	virtual ~XercesXMLFactoryComponent();

	virtual void Dispose();
	virtual IGameXMLHandler *CreateGameXMLHandler() const;
	virtual IGameStateXMLHandler *CreateGameStateXMLHandler(const std::string &,const Version *) const;
	virtual IPreferenceConfigXMLHandler *CreatePreferenceConfigXMLHandler() const;
};

MGDF_CORE_DLL IXMLFactoryComponent *CreateXercesXMLFactoryComponent(HINSTANCE instance,HWND window);

}}}}