#pragma once

#include <map>
#include <string>

namespace MGDF { namespace core { namespace storage {

class IPreferenceConfigStorageHandler
{
public:
	typedef std::map<std::string,std::string>::const_iterator iterator;

	virtual ~IPreferenceConfigStorageHandler(){}
	virtual void Add(const std::string &name,const std::string &value)=0;
	virtual iterator Begin() const=0;
	virtual iterator End() const=0;

	virtual void Load(const std::wstring &load)=0;
	virtual void Save(const std::wstring &save) const=0;
};

}}}