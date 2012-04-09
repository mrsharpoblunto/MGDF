#pragma once

#include <xercesc/sax2/DefaultHandler.hpp>
#include <MGDF/MGDF.hpp>
#include "../MGDFGameStateXMLHandler.hpp"

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

class XercesGameStateXMLHandler: public XERCES_CPP_NAMESPACE::DefaultHandler, public DisposeImpl<IGameStateXMLHandler>
{
public:
	
	XercesGameStateXMLHandler(const std::string &gameUid,const Version *version){
		this->_gameUid = gameUid;
		this->_version = VersionHelper::Copy(version);
	};

	virtual ~XercesGameStateXMLHandler(){};

	virtual void Dispose();

	virtual void Load(const std::wstring &);
	virtual void Save(const std::wstring &) const;
	
	std::string GetGameUid() const { return _gameUid; };
	void SetVersion(const Version *version) { _version =  VersionHelper::Copy(version); };
	const Version *GetVersion() const { return &_version; };

	virtual void startDocument();

	virtual void startElement(
		const XMLCh* const uri, 
		const XMLCh* const localname, 
		const XMLCh* const qname, 
		const XERCES_CPP_NAMESPACE::Attributes& attrs);

	virtual void endElement(
		const XMLCh* const uri, 
		const XMLCh* const localname,
		const XMLCh* const qname);

	virtual void characters(
		const XMLCh* const chars,
		const unsigned int length);

	virtual void warning(const XERCES_CPP_NAMESPACE::SAXParseException& e);
	virtual void error(const XERCES_CPP_NAMESPACE::SAXParseException& e);

private:	
	std::string _currentNode;

	std::string _gameUid;
	Version _version;
};

}}}}