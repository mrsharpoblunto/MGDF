#include "stdafx.h"

#include <boost/filesystem/operations.hpp>

#include <xercesc\framework\MemBufInputSource.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/internal/MemoryManagerImpl.hpp>
#include <xercesc/internal/XMLGrammarpoolImpl.hpp>
#include <xercesc/framework/BinOutputStream.hpp>
#include <xercesc/internal/BinFileOutputStream.hpp>
#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/BinFileInputStream.hpp>
#include <xercesc/util/XMLUni.hpp>
#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFLoggerImpl.hpp"
#include "XercesXmlSchemaCache.hpp"

using namespace XERCES_CPP_NAMESPACE;

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

DEFINE_SINGLETON(XercesXmlSchemaCache)

XercesXmlSchemaCache::XercesXmlSchemaCache(bool doValidation)
{
	_doValidation = doValidation;
	_memMgr = new MemoryManagerImpl();// Create a memory manager instance for memory handling requests.
	_pool = new XMLGrammarPoolImpl(_memMgr);	// Create a grammar ___pool that stores the cached grammars.

	GetLoggerImpl()->Add(THIS_NAME,"building xml schema cache");

	SAX2XMLReader* parser = XMLReaderFactory::createXMLReader(_memMgr, _pool);
	parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
	parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
	parser->setFeature(XMLUni::fgXercesDynamic, false);
	parser->setFeature(XMLUni::fgXercesSchema, true);
	parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
	parser->setFeature(XMLUni::fgXercesCacheGrammarFromParse,true);//cache grammars

	DefaultHandler handler;
	parser->setErrorHandler(&handler);
	parser->setEntityResolver(&handler);

	//load and cache the core schemas
	parser->loadGrammar(Resources::Instance().SchemaFile(Resources::GAME_SCHEMA).c_str(), Grammar::SchemaGrammarType, true);		
	parser->loadGrammar(Resources::Instance().SchemaFile(Resources::GAME_STATE_SCHEMA).c_str(), Grammar::SchemaGrammarType, true);
	parser->loadGrammar(Resources::Instance().SchemaFile(Resources::PREFERENCES_SCHEMA).c_str(), Grammar::SchemaGrammarType, true);

	delete parser;
}

XercesXmlSchemaCache::~XercesXmlSchemaCache(void)
{
	delete _pool;
	delete _memMgr;
}

SAX2XMLReader *XercesXmlSchemaCache::CreateParser() {
	SAX2XMLReader* parser;

	//if schema validation is diabled then create a normal parser
	if (!_doValidation) {
		parser = XMLReaderFactory::createXMLReader();
		parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
	}
	else {//otherwise create a parser that uses the ___pool of cached grammars
		parser = XMLReaderFactory::createXMLReader(_memMgr,_pool);
		parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
		parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
		parser->setFeature(XMLUni::fgXercesDynamic, false);
		parser->setFeature(XMLUni::fgXercesSchema, true);
		parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		parser->setFeature(XMLUni::fgXercesUseCachedGrammarInParse,true);//tell the parser to use the cached grammars
	}
	return parser;
}

void XercesXmlSchemaCache::Parse(XERCES_CPP_NAMESPACE::SAX2XMLReader *parser,IFile *file)
{
	if (file->IsOpen()) {
		file->SetPosition(0);
	}
	else {
		file->OpenFile();
	}

	try {
		char *buffer = new char[file->GetSize()];
		file->Read(buffer,file->GetSize());
		MemBufInputSource *inputSource = new MemBufInputSource((const XMLByte*)buffer,file->GetSize(),"PlaylistXML",false,_memMgr);

		parser->parse(*inputSource);

		delete inputSource;
		delete[] buffer;
	}
	catch (SAXParseException e) {
		file->CloseFile();
		throw e;
	}
	if (file->IsOpen()) {
		file->CloseFile();
	}
}


}}}}