#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFGameStorageHandler.hpp"

namespace MGDF
{
namespace core
{
namespace storage
{
namespace jsoncppImpl
{

class JsonCppGameStorageHandler: public IGameStorageHandler
{
public:
	JsonCppGameStorageHandler() {}
	virtual ~JsonCppGameStorageHandler() {}

	std::string GetGameName() const override final;
	std::string GetGameUid() const override final;
	INT32 GetInterfaceVersion() const override final;
	const Version *GetVersion() const override final;
	const std::map<std::string, std::string> &GetPreferences() const override final;

	MGDFError Load( const std::wstring & ) override final;

private:
	std::string _gameName, _gameUid;
	std::map<std::string, std::string> _preferences;
	INT32 _interfaceVersion;
	Version _version;
};

}
}
}
}