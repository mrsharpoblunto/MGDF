#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

class JsonCppStorageFactoryComponent: public DisposeImpl<IStorageFactoryComponent>
{
public:
	JsonCppStorageFactoryComponent(HINSTANCE instance,HWND window);
	virtual ~JsonCppStorageFactoryComponent();

	virtual void Dispose();
	virtual IGameStorageHandler *CreateGameStorageHandler() const;
	virtual IGameStateStorageHandler *CreateGameStateStorageHandler(const std::string &,const Version *) const;
	virtual IPreferenceConfigStorageHandler *CreatePreferenceConfigStorageHandler() const;
};

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent(HINSTANCE instance,HWND window);

}}}}