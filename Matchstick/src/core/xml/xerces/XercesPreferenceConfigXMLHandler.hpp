#pragma once

#include <xercesc/sax2/DefaultHandler.hpp>
#include <MGDF/MGDF.hpp>
#include "../MGDFPreferenceConfigXMLHandler.hpp"
 
namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

class XercesPreferenceConfigXMLHandler: public XERCES_CPP_NAMESPACE::DefaultHandler, public DisposeImpl<IPreferenceConfigXMLHandler>
{
public:
	XercesPreferenceConfigXMLHandler(){};
	virtual ~XercesPreferenceConfigXMLHandler(){};

	virtual void Dispose();

	virtual void Add(const std::string &name,const std::string &value);
	virtual IPreferenceConfigXMLHandler::iterator Begin() const;
	virtual IPreferenceConfigXMLHandler::iterator End() const;

	virtual void Load(const std::wstring &);
	virtual void Save(const std::wstring &) const;


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
	boost::unordered_map<std::string,std::string> _preferences;
	std::string _name;
	bool _addingName;
	bool _addingValue;
};

}}}}