#pragma once
#include <map>
#include <MGDF/MGDFGame.hpp>
#include "../common/MGDFLoggerImpl.hpp"
#include "../storage/MGDFStorageFactoryComponent.hpp"

namespace MGDF
{
namespace core
{

/**
this class is the concrete implementation of the configuration interface
\author gcconner
*/
class Game : public IGame
{
public:
	Game( const std::string &uid, const std::string &name, INT32 interfaceVersion, const Version *version, storage::IStorageFactoryComponent *xmlFactory );
	virtual ~Game( void ) {}

	const char *GetUid() const override {
		return _uid.c_str();
	}

	const char *GetName() const override {
		return _name.c_str();
	}

	INT32 GetInterfaceVersion() const override {
		return _interfaceVersion;
	}

	const Version *GetVersion() const override {
		return &_version;
	}

	bool HasPreference( const char * name ) const override;
	const char *GetPreference( const char * name ) const override;
	void SetPreference( const char * name, const char * value ) override;
	void SavePreferences() const override;
	void ResetPreferences() override;

	void SavePreferences( const std::wstring &filename );
	void LoadPreferences( const std::wstring &filename );

private:
	storage::IStorageFactoryComponent *_storageFactory;
	std::string _uid, _name;
	std::wstring _preferencesFile;
	Version _version;
	INT32 _interfaceVersion;
	std::map<std::string, std::string> _preferences;

};

}
}