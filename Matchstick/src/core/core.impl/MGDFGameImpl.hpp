#pragma once
#include <hash_map>
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
	Game(std::string uid,std::string name,int interfaceVersion,const Version *version,xml::IXMLFactoryComponent *xmlFactory);
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

	void SavePreferences(std::string filename);
	void LoadPreferences(std::string filename);

private:
	xml::IXMLFactoryComponent *_xmlFactory;
	std::string _uid,_name,_preferencesFile;
	Version _version;
	int _interfaceVersion;
	stdext::hash_map<std::string,std::string> _preferences;

};

}
}