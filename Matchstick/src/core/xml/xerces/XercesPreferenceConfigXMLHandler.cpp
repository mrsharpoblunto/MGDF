#include "stdafx.h"

#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFExceptions.hpp"
#include "XercesXMLSchemaCache.hpp"
#include "XercesUtils.hpp"
#include "XercesPreferenceConfigXMLHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

void XercesPreferenceConfigXMLHandler::Dispose()
{
	delete this;
}

void XercesPreferenceConfigXMLHandler::warning(const SAXParseException& e) 
{  
	GetLoggerImpl()->Add(THIS_NAME,XercesUtils::ToString(e.getMessage()));
}

void XercesPreferenceConfigXMLHandler::error(const SAXParseException& e) 
{ 
	throw new std::exception(XercesUtils::ToString(e.getMessage()).c_str()); 
}

void XercesPreferenceConfigXMLHandler::Add(std::string name,std::string value)
{
	_preferences[name] = value;
}

IPreferenceConfigXMLHandler::iterator XercesPreferenceConfigXMLHandler::Begin() const
{
	return _preferences.begin();
}

IPreferenceConfigXMLHandler::iterator XercesPreferenceConfigXMLHandler::End() const
{
	return _preferences.end();
}

void XercesPreferenceConfigXMLHandler::Load(std::string filename)
{
    try
    {
		SAX2XMLReader *configParser = XercesXmlSchemaCache::Instance().CreateParser();
		configParser->setContentHandler(this);
		configParser->setErrorHandler(this);
		configParser->setEntityResolver(this);
		configParser->parse(filename.c_str());
		delete configParser;
    }
	catch (SAXParseException e) 
	{
		throw MGDFException(XercesUtils::ToString(e.getMessage()));
	}
	catch(std::exception e)
    {
		throw MGDFException(e.what());
    } 
}

void XercesPreferenceConfigXMLHandler::Save(std::string filename) const
{
	std::ofstream file(filename.c_str(),std::ios::out);

	//add xml header and include schema information
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	file << "<mgdf:preferences xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:mgdf=\""<< Resources::PREFERENCES_SCHEMA_URI<<"\">\n";
	
	//save each preference
	for (IPreferenceConfigXMLHandler::iterator iter = _preferences.begin();iter!=_preferences.end();++iter) {
		file << "<preference>\n";
		file << " <name>" << iter->first << "</name>\n";
		file << " <value>" << iter->second << "</value>\n";
		file << "</preference>\n";
	}
	file << "</mgdf:preferences>\n";
	file.close();
}

void XercesPreferenceConfigXMLHandler::startDocument()
{
	this->_name = "";
	this->_addingName = false;
	this->_addingValue = false;
}

void XercesPreferenceConfigXMLHandler::startElement(
				const XMLCh* const uri, 
				const XMLCh* const localname, 
				const XMLCh* const qname, 
				const XERCES_CPP_NAMESPACE::Attributes& attrs)
{
	std::string localName = XercesUtils::ToString(localname);

	if (localName=="name") {
		_addingName = true;
	}
	else if (localName=="value") {
		_addingValue = true;
	}

}

void XercesPreferenceConfigXMLHandler::endElement(
			const XMLCh* const uri, 
			const XMLCh* const localname,
			const XMLCh* const qname)
{
	std::string localName = XercesUtils::ToString(localname);
	if (localName=="name") {
		_addingName = false;
	}
	else if (localName=="value") {
		_addingValue = false;
	}
}

void XercesPreferenceConfigXMLHandler::characters(
			const XMLCh* const chars,
			const unsigned int length)
{
	std::string ch = XercesUtils::ToString(chars);
	if (_addingName) {
		_name = ch;
	}
	else if (_addingValue && _name!="") {
		_preferences[_name] = ch;
	}
}

}}}}