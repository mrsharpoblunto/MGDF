#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

class JsonCppStorageFactoryComponent: public IStorageFactoryComponent
{
public:
	JsonCppStorageFactoryComponent(){}
	virtual ~JsonCppStorageFactoryComponent(){}

	virtual IGameStorageHandler *CreateGameStorageHandler() const;
	virtual IGameStateStorageHandler *CreateGameStateStorageHandler(const std::string &,const Version *) const;
	virtual IPreferenceConfigStorageHandler *CreatePreferenceConfigStorageHandler() const;
};

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent();

}}}}