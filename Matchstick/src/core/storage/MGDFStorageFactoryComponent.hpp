#pragma once

#include "../common/MGDFSystemComponent.hpp"
#include "MGDFGameStorageHandler.hpp"
#include "MGDFGameStateStorageHandler.hpp"
#include "MGDFPreferenceConfigStorageHandler.hpp"

namespace MGDF { namespace core { namespace storage {

class IStorageFactoryComponent: public ISystemComponent
{
public:
	virtual ~IStorageFactoryComponent(){}
	virtual IGameStorageHandler *CreateGameStorageHandler() const=0;
	virtual IGameStateStorageHandler *CreateGameStateStorageHandler(const std::string &game,const Version *version) const=0;
	virtual IPreferenceConfigStorageHandler *CreatePreferenceConfigStorageHandler() const=0;
};

}}}