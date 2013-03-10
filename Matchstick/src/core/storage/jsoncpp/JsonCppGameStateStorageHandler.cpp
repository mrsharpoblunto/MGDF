#include "stdafx.h"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFVersionHelper.hpp"
#include "../../common/MGDFExceptions.hpp"
#include "JsonCppGameStateStorageHandler.hpp"


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

void JsonCppGameStateStorageHandler::Load( const std::wstring &filename )
{
	std::ifstream input( filename.c_str(), std::ios::in );

	Json::Value root;
	Json::Reader reader;

	if ( reader.parse( input, root ) ) {
		_gameUid = root["gameuid"].asString();
		_version = VersionHelper::Create( root["gameversion"].asString() );
	} else {
		throw MGDFException( reader.getFormatedErrorMessages() );
	}
}

void JsonCppGameStateStorageHandler::Save( const std::wstring &filename ) const
{
	std::ofstream file( filename.c_str(), std::ios::out );

	Json::Value root;

	root["gameuid"] = _gameUid;
	root["gameversion"] = VersionHelper::Format( &_version );

	file << root;
	file.close();
}

}
}
}
}