#pragma once

#include <MGDF/MGDFVirtualFileSystem.hpp>
#include "Singleton.hpp"

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/framework/MemoryManager.hpp>
#include <xercesc/framework/XMLGrammarPool.hpp>

namespace MGDF { namespace core { namespace xml { namespace xercesImpl {

/**
this class creates/loads the MGDF core schemas into a cache for faster processing of XML schemas
in the engine, it also provides a means to create parser objects that can access these cached schemas
*/
class XercesXmlSchemaCache: public Singleton<XercesXmlSchemaCache>
{
public:
	XercesXmlSchemaCache();
	~XercesXmlSchemaCache(void);

	DECLARE_SINGLETON(XercesXmlSchemaCache)

	XERCES_CPP_NAMESPACE::SAX2XMLReader *CreateParser();
	void Parse(XERCES_CPP_NAMESPACE::SAX2XMLReader *,IFile *);
private:
	XERCES_CPP_NAMESPACE::MemoryManager *_memMgr;
	XERCES_CPP_NAMESPACE::XMLGrammarPool *_pool;
};

}}}}
