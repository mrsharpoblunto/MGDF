#include "stdafx.h"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFVersionHelper.hpp"
#include "../../common/MGDFExceptions.hpp"
#include "XercesUtils.hpp"
#include "XercesXMLSchemaCache.hpp"
#include "XercesGameStateXMLHandler.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

void XercesGameStateXMLHandler::Dispose()
{
	delete this;
}

void XercesGameStateXMLHandler::warning(const SAXParseException& e) 
{  
	GetLoggerImpl()->Add(THIS_NAME,XercesUtils::ToString(e.getMessage()));
}

void XercesGameStateXMLHandler::error(const SAXParseException& e) 
{ 
	throw std::exception(XercesUtils::ToString(e.getMessage()).c_str()); 
}

void XercesGameStateXMLHandler::Load(const std::wstring &filename)
{
    try
    {
		SAX2XMLReader *loadParser = XercesXmlSchemaCache::Instance().CreateParser();
		loadParser->setContentHandler(this);
		loadParser->setErrorHandler(this);
		loadParser->setEntityResolver(this);
		loadParser->parse(filename.c_str());
		delete loadParser;
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

void XercesGameStateXMLHandler::Save(const std::wstring &filename) const
{
	std::ofstream file(filename.c_str(),std::ios::out);

	//include header info such as the schema, and what configuration/version this save belongs to
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	file << "<mgdf:gameState xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:mgdf=\"" << Resources::GAME_STATE_SCHEMA_URI << "\">\n";
	file << "<gameuid>" << _gameUid << "</gameuid>\n";
	file << "<gameversion>" << VersionHelper::Format(&_version) << "</gameversion>\n";
	file << "</mgdf:gameState>\n";
	file.close();
}

void XercesGameStateXMLHandler::startDocument() 
{
	_currentNode="";
}

void XercesGameStateXMLHandler::startElement(
				const XMLCh* const uri, 
				const XMLCh* const localname, 
				const XMLCh* const qname, 
				const Attributes& attrs)
{
	std::string localName = XercesUtils::ToString(localname);

	if (localName == "gameuid" ||
		localName == "gameversion") 
	{
		 _currentNode = localName;
	}

}

void XercesGameStateXMLHandler::endElement(
				const XMLCh* const uri, 
				const XMLCh* const localname,
				const XMLCh* const qname)
{
	_currentNode = "";
}

void XercesGameStateXMLHandler::characters(				
				const XMLCh* const chars,
				const unsigned int length)
{
	std::string ch = XercesUtils::ToString(chars);

	if (_currentNode=="gameuid") {
		if (ch!=_gameUid) {
			throw MGDFException("This game states target game uid is \""+ch+"\" which is different to the expected game uid \""+_gameUid+"\"");
		}
	}
	else if (_currentNode=="gameversion") {
		Version psVersion = VersionHelper::Create(ch);
		_version = psVersion;
	}
}

}}}}