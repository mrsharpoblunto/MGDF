#include "stdafx.h"

#include <boost/lexical_cast.hpp>

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFVersionHelper.hpp"
#include "../../common/MGDFExceptions.hpp"
#include "XercesXmlSchemaCache.hpp"
#include "XercesGameXMLHandler.hpp"
#include "XercesUtils.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

void XercesGameXMLHandler::Dispose()
{
	delete this;
}

void XercesGameXMLHandler::warning(const SAXParseException& e) 
{  
	GetLoggerImpl()->Add(THIS_NAME,XercesUtils::ToString(e.getMessage()));
}

void XercesGameXMLHandler::error(const SAXParseException& e) 
{ 
	throw std::exception(XercesUtils::ToString(e.getMessage()).c_str()); 
}

std::string XercesGameXMLHandler::GetGameName() const
{
	return _gameName;
}

std::string XercesGameXMLHandler::GetGameUid() const
{
	return _gameUid;
}

int XercesGameXMLHandler::GetInterfaceVersion() const
{
	return _interfaceVersion;
}

const Version *XercesGameXMLHandler::GetVersion() const
{
	return &_version;
}

void XercesGameXMLHandler::Load(std::string filename)
{
	try {
		SAX2XMLReader *bootParser = XercesXmlSchemaCache::Instance().CreateParser();
		bootParser->setContentHandler(this);
		bootParser->setErrorHandler(this);
		bootParser->setEntityResolver(this);
		bootParser->parse(filename.c_str());
		delete bootParser;
	}
	catch (SAXParseException e) {
		throw MGDFException(XercesUtils::ToString(e.getMessage()));
	}
	catch(std::exception e)
    {
		throw MGDFException(e.what());
    } 
}

void XercesGameXMLHandler::startDocument()
{
	_currentNode="";
	_parameterString = "";
}

void XercesGameXMLHandler::startElement(
				const XMLCh* const uri, 
				const XMLCh* const localname, 
				const XMLCh* const qname, 
				const XERCES_CPP_NAMESPACE::Attributes& attrs)
{
	std::string localName = XercesUtils::ToString(localname);

	//adding configuration and parameter info
	if (localName == "gamename" ||
			localName == "gameuid" ||
			 localName == "parameters" ||
			 localName == "interfaceversion" ||
			 localName == "version") {
		 _currentNode = localName;
	}
}

void XercesGameXMLHandler::endElement(
				const XMLCh* const uri, 
				const XMLCh* const localname,
				const XMLCh* const qname)
{
	std::string localName = XercesUtils::ToString(localname);
	_currentNode="";//reset the current node to nothing

}

void XercesGameXMLHandler::characters(				
				const XMLCh* const chars,
				const unsigned int length)
{
	std::string ch = XercesUtils::ToString(chars);
	//adding configuration and parameter info
	if (_currentNode == "gamename") {
		_gameName = ch;
	}
	else if (_currentNode == "gameuid") {
		_gameUid = ch;
	}
	else if (_currentNode == "version") {
		_version = VersionHelper::Create(ch);
	}
	else if (_currentNode == "parameters") {
		_parameterString = ch;
	}
	else if (_currentNode == "interfaceversion") {
		_interfaceVersion = boost::lexical_cast<int>(ch);
	}
}

std::string XercesGameXMLHandler::GetParameterString() const
{
	return _parameterString;
}

}}}}