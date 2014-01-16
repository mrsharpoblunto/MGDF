#include "stdafx.h"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFVersionHelper.hpp"
#include "../../common/MGDFExceptions.hpp"
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

void JsonCppGameStorageHandler::Load( const std::wstring &filename )
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
	} else {
		throw MGDFException( MGDF_ERR_INVALID_JSON, reader.getFormatedErrorMessages() );
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