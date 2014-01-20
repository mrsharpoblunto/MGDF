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

	void Add( const std::string &name, const std::string &value ) override;
	IPreferenceConfigStorageHandler::iterator begin() const override;
	IPreferenceConfigStorageHandler::iterator end() const override;

	MGDFError Load( const std::wstring & ) override;
	void Save( const std::wstring & ) const override;
private:
	std::map<std::string, std::string> _preferences;
};

}
}
}
}