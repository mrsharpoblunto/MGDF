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

	std::string GetGameName() const override;
	std::string GetGameUid() const override;
	INT32 GetInterfaceVersion() const override;
	const Version *GetVersion() const override;
	std::string GetParameterString() const override;

	MGDFError Load( const std::wstring & ) override;

private:
	std::string _gameName, _gameUid, _parameterString;
	INT32 _interfaceVersion;
	Version _version;
};

}
}
}
}