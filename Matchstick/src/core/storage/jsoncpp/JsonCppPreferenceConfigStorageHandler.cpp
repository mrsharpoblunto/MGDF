#include "stdafx.h"

#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFLoggerImpl.hpp"

#include "JsonCppPreferenceConfigStorageHandler.hpp"


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

void JsonCppPreferenceConfigStorageHandler::Add( const std::string &name, const std::string &value )
{
	_preferences[name] = value;
}

IPreferenceConfigStorageHandler::iterator JsonCppPreferenceConfigStorageHandler::begin() const
{
	return _preferences.begin();
}

IPreferenceConfigStorageHandler::iterator JsonCppPreferenceConfigStorageHandler::end() const
{
	return _preferences.end();
}

MGDFError JsonCppPreferenceConfigStorageHandler::Load( const std::wstring &filename )
{
	std::ifstream input( filename.c_str(), std::ios::in );

	Json::Value root;
	Json::Reader reader;

	if ( reader.parse( input, root ) ) {
		for (const auto& name : root.getMemberNames()) {
			if (!root[name].isString()) {
				LOG(reader.getFormatedErrorMessages(), LOG_ERROR);
				return MGDF_ERR_INVALID_JSON;
			}
			_preferences[name] = root[name].asString();
		}
		return MGDF_OK;
	} else {
		LOG( reader.getFormatedErrorMessages(), LOG_ERROR );
		return MGDF_ERR_INVALID_JSON;
	}
}

void JsonCppPreferenceConfigStorageHandler::Save( const std::wstring &filename ) const
{
	std::ofstream file( filename.c_str(), std::ios::out );

	Json::Value root;

	for ( auto &pref : _preferences ) {
		root[pref.first] = pref.second;
	}

	file << root;
	file.close();
}

}
}
}
}