#pragma once

#include <MGDF/MGDFDisposable.hpp>
#include <xutility>
#include <boost/unordered_map.hpp>
#include <string>

namespace MGDF { namespace core { namespace xml {

class IPreferenceConfigXMLHandler: public IDisposable
{
public:
	typedef boost::unordered_map<std::string,std::string>::const_iterator iterator;

	virtual void Add(const std::string &name,const std::string &value)=0;
	virtual iterator Begin() const=0;
	virtual iterator End() const=0;

	virtual void Load(const std::wstring &load)=0;
	virtual void Save(const std::wstring &save) const=0;
};

}}}