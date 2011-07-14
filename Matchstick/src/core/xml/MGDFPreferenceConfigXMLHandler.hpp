#pragma once

#include <MGDF/MGDFDisposable.hpp>
#include <xutility>
#include <hash_map>
#include <string>

namespace MGDF { namespace core { namespace xml {

class IPreferenceConfigXMLHandler: public IDisposable
{
public:
	typedef stdext::hash_map<std::string,std::string>::const_iterator iterator;

	virtual void Add(std::string name,std::string value)=0;
	virtual iterator Begin() const=0;
	virtual iterator End() const=0;

	virtual void Load(std::wstring)=0;
	virtual void Save(std::wstring) const=0;
};

}}}