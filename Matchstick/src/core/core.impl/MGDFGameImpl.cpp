#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFGameImpl.hpp"
#include "MGDFSystemImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

Game::Game(const std::string &uid,const std::string &name,INT32 interfaceVersion,const Version *version,storage::IStorageFactoryComponent *storageFactory)
	: _uid(uid)
	, _name(name)
	, _interfaceVersion(interfaceVersion)
	, _version(VersionHelper::Copy(version))
	, _storageFactory(storageFactory)
{
	_ASSERTE(storageFactory);
}

bool Game::HasPreference(const char *name) const
{
	return _preferences.find(name)!=_preferences.end();
}

const char *Game::GetPreference(const char *name) const
{
	_ASSERTE(name);
	auto iter = _preferences.find(name);
	if (iter!=_preferences.end()) {
		return iter->second.c_str();
	}
	else {
		return nullptr;
	}
}

void Game::SetPreference(const char *name,const char *value)
{
	_ASSERTE(name);
	_ASSERTE(value);
	auto it = _preferences.find(name);
	if (it!=_preferences.end()) {
		it->second = value;
	}
}

void Game::ResetPreferences()
{
	LoadPreferences(_preferencesFile.c_str());
}

void Game::SavePreferences() const
{
	std::auto_ptr<storage::IPreferenceConfigStorageHandler> handler(_storageFactory->CreatePreferenceConfigStorageHandler());
	for (auto iter = _preferences.begin();iter!=_preferences.end();++iter) {
		handler->Add(iter->first,iter->second);
	}
	handler->Save(_preferencesFile);
	LOG("saved preferences to '" << Resources::ToString(_preferencesFile) << "' successfully",LOG_MEDIUM);
}

void Game::SavePreferences(const std::wstring &filename) {
	_preferencesFile = filename;
	SavePreferences();
}

void Game::LoadPreferences(const std::wstring &filename)
{
	std::auto_ptr<storage::IPreferenceConfigStorageHandler> handler(_storageFactory->CreatePreferenceConfigStorageHandler());
	handler->Load(filename);
	for (auto iter = handler->Begin();iter!=handler->End();++iter)
	{
		_preferences[iter->first] = iter->second;
	}
	LOG("loaded preferences from '"+Resources::ToString(filename)+"' successfully",LOG_MEDIUM);
}

}
}