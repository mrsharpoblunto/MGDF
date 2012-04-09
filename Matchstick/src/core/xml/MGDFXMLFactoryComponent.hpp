#pragma once

#include <MGDF/MGDFDisposable.hpp>
#include "../common/MGDFSystemComponent.hpp"
#include "MGDFGameXMLHandler.hpp"
#include "MGDFGameStateXMLHandler.hpp"
#include "MGDFPreferenceConfigXMLHandler.hpp"

namespace MGDF { namespace core { namespace xml {

class IXMLFactoryComponent: public ISystemComponent,public IDisposable
{
public:
	virtual IGameXMLHandler *CreateGameXMLHandler() const=0;
	virtual IGameStateXMLHandler *CreateGameStateXMLHandler(const std::string &game,const Version *version) const=0;
	virtual IPreferenceConfigXMLHandler *CreatePreferenceConfigXMLHandler() const=0;
};

}}}