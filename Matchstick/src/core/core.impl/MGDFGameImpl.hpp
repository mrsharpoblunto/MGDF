#pragma once
#include <boost/unordered_map.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <MGDF/MGDFSystem.hpp>
#include <MGDF/MGDFGame.hpp>
#include "../common/MGDFLoggerImpl.hpp"
#include "../xml/MGDFXMLFactoryComponent.hpp"

namespace MGDF { namespace core {

/**
this class is the concrete implementation of the configuration interface
\author gcconner
*/
class Game : public IGame
{
public:
	Game(const std::string &uid,const std::string &name,int interfaceVersion,const Version *version,xml::IXMLFactoryComponent *xmlFactory);
	virtual ~Game(void);

	virtual const char *GetUid() const;
	virtual const char *GetName() const;
	virtual int GetInterfaceVersion() const;
	virtual const Version *GetVersion() const;

	virtual bool HasPreference(const char * name) const;
	virtual const char *GetPreference(const char * name) const;
	virtual void SetPreference(const char * name,const char * value);
	virtual void SavePreferences() const;
	virtual void ResetPreferences();

	void SavePreferences(const std::wstring &filename);
	void LoadPreferences(const std::wstring &filename);

private:
	xml::IXMLFactoryComponent *_xmlFactory;
	std::string _uid,_name;
	std::wstring _preferencesFile;
	Version _version;
	int _interfaceVersion;
	boost::unordered_map<std::string,std::string> _preferences;

};

}
}