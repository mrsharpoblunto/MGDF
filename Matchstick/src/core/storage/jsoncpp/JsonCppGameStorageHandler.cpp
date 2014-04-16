#include "stdafx.h"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFVersionHelper.hpp"
#include "JsonCppGameStorageHandler.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{
namespace storage
{
namespace jsoncppImpl
{

std::string JsonCppGameStorageHandler::GetGameName() const
{
	return _gameName;
}

std::string JsonCppGameStorageHandler::GetGameUid() const
{
	return _gameUid;
}

INT32 JsonCppGameStorageHandler::GetInterfaceVersion() const
{
	return _interfaceVersion;
}

const Version *JsonCppGameStorageHandler::GetVersion() const
{
	return &_version;
}

const std::map<std::string, std::string> &JsonCppGameStorageHandler::GetPreferences() const
{
	return _preferences;
}

MGDFError JsonCppGameStorageHandler::Load( const std::wstring &filename )
{
	std::ifstream input( filename.c_str(), std::ios::in );

	Json::Value root;
	Json::Reader reader;

	if ( reader.parse( input, root ) ) {
		_gameName = root["gamename"].asString();
		_gameUid = root["gameuid"].asString();
		_version = VersionHelper::Create( root["version"].asString() );
		_parameterString = root["parameters"].asString();
		_interfaceVersion = atoi( root["interfaceversion"].asString().c_str() );
		Json::Value preferences = root["preferences"];
		if (!preferences.isNull()) {
			for (const auto &key : preferences.getMemberNames()) {
				_preferences.insert(std::make_pair(key, preferences[key].asString()));
			}
		}
		return MGDF_OK;
	} else {
		LOG( reader.getFormatedErrorMessages(), LOG_ERROR );
		return MGDF_ERR_INVALID_JSON;
	}
}

std::string JsonCppGameStorageHandler::GetParameterString() const
{
	return _parameterString;
}

}
}
}
}