#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFPreferenceConfigStorageHandler.hpp"

namespace MGDF
{
namespace core
{
namespace storage
{
namespace jsoncppImpl
{

class JsonCppPreferenceConfigStorageHandler: public IPreferenceConfigStorageHandler
{
public:
	JsonCppPreferenceConfigStorageHandler() {}
	virtual ~JsonCppPreferenceConfigStorageHandler() {}

	void Add( const std::string &name, const std::string &value ) override final;
	IPreferenceConfigStorageHandler::iterator begin() const override final;
	IPreferenceConfigStorageHandler::iterator end() const override final;

	MGDFError Load( const std::wstring & ) override final;
	void Save( const std::wstring & ) const override final;
private:
	std::map<std::string, std::string> _preferences;
};

}
}
}
}