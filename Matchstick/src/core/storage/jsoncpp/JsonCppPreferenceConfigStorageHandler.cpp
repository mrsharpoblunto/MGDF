#include "stdafx.h"

#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFExceptions.hpp"

#include "JsonCppPreferenceConfigStorageHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

void JsonCppPreferenceConfigStorageHandler::Dispose()
{
	delete this;
}

void JsonCppPreferenceConfigStorageHandler::Add(const std::string &name,const std::string &value)
{
	_preferences[name] = value;
}

IPreferenceConfigStorageHandler::iterator JsonCppPreferenceConfigStorageHandler::Begin() const
{
	return _preferences.begin();
}

IPreferenceConfigStorageHandler::iterator JsonCppPreferenceConfigStorageHandler::End() const
{
	return _preferences.end();
}

void JsonCppPreferenceConfigStorageHandler::Load(const std::wstring &filename)
{
	std::ifstream input(filename.c_str(),std::ios::in);

	Json::Value root;
	Json::Reader reader;

	if (reader.parse(input,root))
	{
		Json::Value preferences = root["preferences"];
		for ( unsigned int index = 0; index < preferences.size(); ++index )
		{
			_preferences[preferences[index]["name"].asString()] = preferences[index]["value"].asString();
		}
	}
	else
	{
		throw MGDFException(reader.getFormatedErrorMessages());
	}
}

void JsonCppPreferenceConfigStorageHandler::Save(const std::wstring &filename) const
{
	std::ofstream file(filename.c_str(),std::ios::out);

	Json::Value root;
	Json::Value preferences;

	for (IPreferenceConfigStorageHandler::iterator iter = _preferences.begin();iter!=_preferences.end();++iter) {
		Json::Value pref;
		pref["name"] = iter->first;
		pref["value"] = iter->second;
		preferences.append(pref);
	}
	root["preferences"] = preferences;

	file << root;
	file.close();
}

}}}}