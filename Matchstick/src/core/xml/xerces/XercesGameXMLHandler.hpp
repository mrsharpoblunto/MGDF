#pragma once

#include <xercesc/sax2/DefaultHandler.hpp>
#include <MGDF/MGDF.hpp>
#include "../MGDFGameXMLHandler.hpp"

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

class XercesGameXMLHandler: public XERCES_CPP_NAMESPACE::DefaultHandler, public DisposeImpl<IGameXMLHandler> 
{
public:
	XercesGameXMLHandler(){};
	virtual ~XercesGameXMLHandler(){};

	virtual void Dispose();

	virtual std::string GetGameName() const;
	virtual std::string GetGameUid() const;
	virtual int GetInterfaceVersion() const;
	virtual const Version *GetVersion() const;
	virtual std::string GetParameterString() const;

	virtual void Load(const std::wstring &);

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
	std::string _gameName,_gameUid,_parameterString;
	int _interfaceVersion;
	Version _version;
};

}}}}