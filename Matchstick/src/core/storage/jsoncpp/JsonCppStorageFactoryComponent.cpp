#include "stdafx.h"

#include "JsonCppStorageFactoryComponent.hpp"
#include "JsonCppGameStorageHandler.hpp"
#include "JsonCppGameStateStorageHandler.hpp"
#include "JsonCppPreferenceConfigStorageHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent(HINSTANCE instance,HWND window)
{
	try {
		return new JsonCppStorageFactoryComponent(instance,window);
	}
	catch (...)
	{
		return nullptr;
	}
}

JsonCppStorageFactoryComponent::JsonCppStorageFactoryComponent(HINSTANCE instance,HWND window)
{
}

void JsonCppStorageFactoryComponent::Dispose()
{
	delete this;
}

JsonCppStorageFactoryComponent::~JsonCppStorageFactoryComponent()
{
}

IGameStorageHandler *JsonCppStorageFactoryComponent::CreateGameStorageHandler() const
{
	return new JsonCppGameStorageHandler();
}

IGameStateStorageHandler *JsonCppStorageFactoryComponent::CreateGameStateStorageHandler(const std::string &game,const Version *version) const
{
	return new JsonCppGameStateStorageHandler(game,version);
}

IPreferenceConfigStorageHandler *JsonCppStorageFactoryComponent::CreatePreferenceConfigStorageHandler() const
{
	return new JsonCppPreferenceConfigStorageHandler();
}

}}}}