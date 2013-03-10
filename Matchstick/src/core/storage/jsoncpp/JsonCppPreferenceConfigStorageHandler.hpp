#pragma once

#include <MGDF/MGDF.hpp>
#include "../MGDFPreferenceConfigStorageHandler.hpp"
 
namespace MGDF { namespace core { namespace storage { namespace jsoncppImpl {

class JsonCppPreferenceConfigStorageHandler: public IPreferenceConfigStorageHandler
{
public:
	JsonCppPreferenceConfigStorageHandler(){}
	virtual ~JsonCppPreferenceConfigStorageHandler(){}

	virtual void Add(const std::string &name,const std::string &value);
	virtual IPreferenceConfigStorageHandler::iterator Begin() const;
	virtual IPreferenceConfigStorageHandler::iterator End() const;

	virtual void Load(const std::wstring &);
	virtual void Save(const std::wstring &) const;
private:
	std::map<std::string,std::string> _preferences;
};

}}}}