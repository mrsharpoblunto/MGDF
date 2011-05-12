/*
 * Copyright 2002,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: XSDErrorReporter.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/XMLErrorCodes.hpp>
#include <xercesc/framework/XMLValidityCodes.hpp>
#include <xercesc/framework/XMLErrorReporter.hpp>
#include <xercesc/util/XMLMsgLoader.hpp>
#include <xercesc/util/XMLRegisterCleanup.hpp>
#include <xercesc/util/XMLInitializer.hpp>
#include <xercesc/validators/schema/XSDErrorReporter.hpp>
#include <xercesc/validators/schema/XSDLocator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Local static data
// ---------------------------------------------------------------------------
static XMLMsgLoader*  gErrMsgLoader = 0;
static XMLMsgLoader*  gValidMsgLoader = 0;
static XMLMutex*      sErrRprtrMutex = 0;
static XMLRegisterCleanup errRprtrMutexCleanup;
static XMLRegisterCleanup cleanupErrMsgLoader;
static XMLRegisterCleanup cleanupValidMsgLoader;

// ---------------------------------------------------------------------------
//  Local, static functions
// ---------------------------------------------------------------------------
static void reinitErrRprtrMutex()
{
    delete sErrRprtrMutex;
    sErrRprtrMutex = 0;
}

static XMLMutex& getErrRprtrMutex()
{
    if (!sErrRprtrMutex)
    {
        XMLMutexLock lockInit(XMLPlatformUtils::fgAtomicMutex);

        if (!sErrRprtrMutex)
        {
            sErrRprtrMutex = new XMLMutex(XMLPlatformUtils::fgMemoryManager);
            errRprtrMutexCleanup.registerCleanup(reinitErrRprtrMutex);
        }
    }

    return *sErrRprtrMutex;
}

static void reinitErrMsgLoader()
{
	delete gErrMsgLoader;
	gErrMsgLoader = 0;
}

static void reinitValidMsgLoader()
{
	delete gValidMsgLoader;
	gValidMsgLoader = 0;
}

static XMLMsgLoader* getErrMsgLoader()
{
    if (!gErrMsgLoader)
    {
        XMLMutexLock lock(&getErrRprtrMutex());

        if (!gErrMsgLoader)
        {
            gErrMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgXMLErrDomain);

            if (!gErrMsgLoader)
                XMLPlatformUtils::panic(PanicHandler::Panic_CantLoadMsgDomain);
            else
                cleanupErrMsgLoader.registerCleanup(reinitErrMsgLoader);
        }
    }

    return gErrMsgLoader;
}


static XMLMsgLoader* getValidMsgLoader()
{
    if (!gValidMsgLoader)
    {
        XMLMutexLock lock(&getErrRprtrMutex());

        if (!gValidMsgLoader)
        {
            gValidMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgValidityDomain);

            if (!gValidMsgLoader)
                XMLPlatformUtils::panic(PanicHandler::Panic_CantLoadMsgDomain);
            else
                cleanupValidMsgLoader.registerCleanup(reinitValidMsgLoader);
        }
    }
    return gValidMsgLoader;
}

void XMLInitializer::initializeXSDErrReporterMsgLoader()
{
    gErrMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgXMLErrDomain);
    if (gErrMsgLoader) {
        cleanupErrMsgLoader.registerCleanup(reinitErrMsgLoader);
    }

    gValidMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgValidityDomain);
    if (gValidMsgLoader) {
        cleanupValidMsgLoader.registerCleanup(reinitValidMsgLoader);
    }
}

// ---------------------------------------------------------------------------
//  XSDErrorReporter: Constructors and Destructor
// ---------------------------------------------------------------------------
XSDErrorReporter::XSDErrorReporter(XMLErrorReporter* const errorReporter) :
    fExitOnFirstFatal(false)
    , fErrorReporter(errorReporter)
{

}


// ---------------------------------------------------------------------------
//  XSDErrorReporter: Error reporting
// ---------------------------------------------------------------------------
void XSDErrorReporter::emitError(const unsigned int toEmit,
                                 const XMLCh* const msgDomain,
                                 const Locator* const aLocator)
{
    // Bump the error count if it is not a warning
//    if (XMLErrs::errorType(toEmit) != XMLErrorReporter::ErrType_Warning)
//        incrementErrorCount();

    //
    //  Load the message into alocal and replace any tokens found in
    //  the text.
    //
    const unsigned int msgSize = 1023;
    XMLCh errText[msgSize + 1];
    XMLMsgLoader* msgLoader = getErrMsgLoader();
    XMLErrorReporter::ErrTypes errType = XMLErrs::errorType((XMLErrs::Codes) toEmit);

    if (XMLString::equals(msgDomain, XMLUni::fgValidityDomain)) {

        errType = XMLValid::errorType((XMLValid::Codes) toEmit);
        msgLoader = getValidMsgLoader();
    }

    if (!msgLoader->loadMsg(toEmit, errText, msgSize))
    {
                // <TBD> Should probably load a default message here
    }

    if (fErrorReporter)
        fErrorReporter->error(toEmit, msgDomain, errType, errText, aLocator->getSystemId(),
                              aLocator->getPublicId(), aLocator->getLineNumber(),
                              aLocator->getColumnNumber());

    // Bail out if its fatal an we are to give up on the first fatal error
    if (errType == XMLErrorReporter::ErrType_Fatal && fExitOnFirstFatal)
        throw (XMLErrs::Codes) toEmit;
}

void XSDErrorReporter::emitError(const unsigned int toEmit,
                                 const XMLCh* const msgDomain,
                                 const Locator* const aLocator,
                                 const XMLCh* const text1,
                                 const XMLCh* const text2,
                                 const XMLCh* const text3,
                                 const XMLCh* const text4,
                                 MemoryManager* const manager)
{
    // Bump the error count if it is not a warning
//    if (XMLErrs::errorType(toEmit) != XMLErrorReporter::ErrType_Warning)
//        incrementErrorCount();

    //
    //  Load the message into alocal and replace any tokens found in
    //  the text.
    //
    const unsigned int maxChars = 2047;
    XMLCh errText[maxChars + 1];
    XMLMsgLoader* msgLoader = getErrMsgLoader();
    XMLErrorReporter::ErrTypes errType = XMLErrs::errorType((XMLErrs::Codes) toEmit);

    if (XMLString::equals(msgDomain, XMLUni::fgValidityDomain)) {

        errType = XMLValid::errorType((XMLValid::Codes) toEmit);
        msgLoader = getValidMsgLoader();
    }

    if (!msgLoader->loadMsg(toEmit, errText, maxChars, text1, text2, text3, text4, manager))
    {
                // <TBD> Should probably load a default message here
    }

    if (fErrorReporter)
        fErrorReporter->error(toEmit, msgDomain, errType, errText, aLocator->getSystemId(),
                              aLocator->getPublicId(), aLocator->getLineNumber(),
                              aLocator->getColumnNumber());

    // Bail out if its fatal an we are to give up on the first fatal error
    if (errType == XMLErrorReporter::ErrType_Fatal && fExitOnFirstFatal)
        throw (XMLErrs::Codes) toEmit;
}

XERCES_CPP_NAMESPACE_END
