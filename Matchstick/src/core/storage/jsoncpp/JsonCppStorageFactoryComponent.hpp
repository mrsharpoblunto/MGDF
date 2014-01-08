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

	IGameStorageHandler *CreateGameStorageHandler() const override;
	IGameStateStorageHandler *CreateGameStateStorageHandler( const std::string &, const Version * ) const override;
	IPreferenceConfigStorageHandler *CreatePreferenceConfigStorageHandler() const override;
};

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent();

}
}
}
}