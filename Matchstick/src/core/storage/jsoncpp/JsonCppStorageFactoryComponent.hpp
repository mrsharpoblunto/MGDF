#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF
{
namespace core
{
namespace storage
{
namespace jsoncppImpl
{

class JsonCppStorageFactoryComponent: public IStorageFactoryComponent
{
public:
	JsonCppStorageFactoryComponent() {}
	virtual ~JsonCppStorageFactoryComponent() {}

	IGameStorageHandler *CreateGameStorageHandler() const override final;
	IGameStateStorageHandler *CreateGameStateStorageHandler( const std::string &, const Version * ) const override final;
	IPreferenceConfigStorageHandler *CreatePreferenceConfigStorageHandler() const override final;
};

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent();

}
}
}
}