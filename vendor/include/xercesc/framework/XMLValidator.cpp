/*
 * Copyright 1999-2004 The Apache Software Foundation.
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
 * $Id: XMLValidator.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/framework/XMLValidator.hpp>
#include <xercesc/util/Mutexes.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLInitializer.hpp>
#include <xercesc/util/XMLMsgLoader.hpp>
#include <xercesc/util/XMLRegisterCleanup.hpp>
#include <xercesc/internal/XMLScanner.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Local static functions
// ---------------------------------------------------------------------------

static XMLMutex* sMsgMutex = 0;
static XMLRegisterCleanup msgLoaderCleanup;

static XMLMsgLoader* sMsgLoader = 0;
static XMLRegisterCleanup validatorMutexCleanup;

//
//  We need to fault in this mutex. But, since its used for synchronization
//  itself, we have to do this the low level way using a compare and swap.
//
static XMLMutex& gValidatorMutex()
{
    if (!sMsgMutex)
    {
        XMLMutexLock lockInit(XMLPlatformUtils::fgAtomicMutex);

        if (!sMsgMutex)
        {
            sMsgMutex = new (XMLPlatformUtils::fgMemoryManager) XMLMutex(XMLPlatformUtils::fgMemoryManager);
            validatorMutexCleanup.registerCleanup(XMLValidator::reinitMsgMutex);
        }
    }

    return *sMsgMutex;
}

static XMLMsgLoader& getMsgLoader()
{
    if (!sMsgLoader)
    {
	    // Lock the mutex
	    XMLMutexLock lockInit(&gValidatorMutex());

        if (!sMsgLoader)
		{
		    sMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgValidityDomain);
		    if (!sMsgLoader)
			    XMLPlatformUtils::panic(PanicHandler::Panic_CantLoadMsgDomain);

            //
            // Register this XMLMsgLoader for cleanup at Termination.
            //
            msgLoaderCleanup.registerCleanup(XMLValidator::reinitMsgLoader);
        }
    }

    return *sMsgLoader;
}

void XMLInitializer::initializeValidatorMsgLoader()
{
    sMsgLoader = XMLPlatformUtils::loadMsgSet(XMLUni::fgValidityDomain);

    // Register this XMLMsgLoader for cleanup at Termination.
    if (sMsgLoader) {
        msgLoaderCleanup.registerCleanup(XMLValidator::reinitMsgLoader);
    }
}

// ---------------------------------------------------------------------------
//  XMLValidator: Error emitting methods
// ---------------------------------------------------------------------------

//
//  These methods are called whenever the scanner wants to emit an error.
//  It handles getting the message loaded, doing token replacement, etc...
//  and then calling the error handler, if its installed.
//
void XMLValidator::emitError(const XMLValid::Codes toEmit)
{
    // Bump the error count if it is not a warning
    if (XMLValid::errorType(toEmit) != XMLErrorReporter::ErrType_Warning)
        fScanner->incrementErrorCount();

    //	Call error reporter if we have one
    if (fErrorReporter)
    {
        // Load the message into a local for display
        const unsigned int msgSize = 1023;
        XMLCh errText[msgSize + 1];

        // load the text
		if (!getMsgLoader().loadMsg(toEmit, errText, msgSize))
		{
			// <TBD> Probably should load a default msg here
        }

        //
        //  Create a LastExtEntityInfo structure and get the reader manager
        //  to fill it in for us. This will give us the information about
        //  the last reader on the stack that was an external entity of some
        //  sort (i.e. it will ignore internal entities.
        //
        ReaderMgr::LastExtEntityInfo lastInfo;
        fReaderMgr->getLastExtEntityInfo(lastInfo);

        fErrorReporter->error
        (
            toEmit
            , XMLUni::fgValidityDomain
            , XMLValid::errorType(toEmit)
            , errText
            , lastInfo.systemId
            , lastInfo.publicId
            , lastInfo.lineNumber
            , lastInfo.colNumber
        );
    }

    // Bail out if its fatal an we are to give up on the first fatal error
    if (((XMLValid::isError(toEmit) && fScanner->getValidationConstraintFatal())
         || XMLValid::isFatal(toEmit))
    &&  fScanner->getExitOnFirstFatal()
    &&  !fScanner->getInException())
    {
        throw toEmit;
    }
}

void XMLValidator::emitError(const  XMLValid::Codes toEmit
                            , const XMLCh* const    text1
                            , const XMLCh* const    text2
                            , const XMLCh* const    text3
                            , const XMLCh* const    text4)
{
    // Bump the error count if it is not a warning
    if (XMLValid::errorType(toEmit) != XMLErrorReporter::ErrType_Warning)
        fScanner->incrementErrorCount();

    //	Call error reporter if we have one
    if (fErrorReporter)
    {
        //
        //  Load the message into alocal and replace any tokens found in
        //  the text.
        //
        const unsigned int maxChars = 2047;
        XMLCh errText[maxChars + 1];

        // load the text
		if (!getMsgLoader().loadMsg(toEmit, errText, maxChars, text1, text2, text3, text4, fScanner->getMemoryManager()))
		{
			// <TBD> Should probably load a default message here
        }

        //
        //  Create a LastExtEntityInfo structure and get the reader manager
        //  to fill it in for us. This will give us the information about
        //  the last reader on the stack that was an external entity of some
        //  sort (i.e. it will ignore internal entities.
        //
        ReaderMgr::LastExtEntityInfo lastInfo;
        fReaderMgr->getLastExtEntityInfo(lastInfo);

        fErrorReporter->error
        (
            toEmit
            , XMLUni::fgValidityDomain
            , XMLValid::errorType(toEmit)
            , errText
            , lastInfo.systemId
            , lastInfo.publicId
            , lastInfo.lineNumber
            , lastInfo.colNumber
        );
    }

    // Bail out if its fatal an we are to give up on the first fatal error
    if (((XMLValid::isError(toEmit) && fScanner->getValidationConstraintFatal())
         || XMLValid::isFatal(toEmit))
    &&  fScanner->getExitOnFirstFatal()
    &&  !fScanner->getInException())
    {
        throw toEmit;
    }
}

void XMLValidator::emitError(const  XMLValid::Codes toEmit
                            , const char* const     text1
                            , const char* const     text2
                            , const char* const     text3
                            , const char* const     text4)
{
    // Bump the error count if it is not a warning
    if (XMLValid::errorType(toEmit) != XMLErrorReporter::ErrType_Warning)
        fScanner->incrementErrorCount();

    //	Call error reporter if we have one
    if (fErrorReporter)
    {
        //
        //  Load the message into alocal and replace any tokens found in
        //  the text.
        //
        const unsigned int maxChars = 2047;
        XMLCh errText[maxChars + 1];

        // load the text
		if (!getMsgLoader().loadMsg(toEmit, errText, maxChars, text1, text2, text3, text4, fScanner->getMemoryManager()))
		{
			// <TBD> Should probably load a default message here
        }

        //
        //  Create a LastExtEntityInfo structure and get the reader manager
        //  to fill it in for us. This will give us the information about
        //  the last reader on the stack that was an external entity of some
        //  sort (i.e. it will ignore internal entities.
        //
        ReaderMgr::LastExtEntityInfo lastInfo;
        fReaderMgr->getLastExtEntityInfo(lastInfo);

        fErrorReporter->error
        (
            toEmit
            , XMLUni::fgValidityDomain
            , XMLValid::errorType(toEmit)
            , errText
            , lastInfo.systemId
            , lastInfo.publicId
            , lastInfo.lineNumber
            , lastInfo.colNumber
        );
    }

    // Bail out if its fatal an we are to give up on the first fatal error
    if (((XMLValid::isError(toEmit) && fScanner->getValidationConstraintFatal())
         || XMLValid::isFatal(toEmit))
    &&  fScanner->getExitOnFirstFatal()
    &&  !fScanner->getInException())
    {
        throw toEmit;
    }
}


// ---------------------------------------------------------------------------
//  XMLValidator: Hidden Constructors
// ---------------------------------------------------------------------------
XMLValidator::XMLValidator(XMLErrorReporter* const errReporter) :

    fBufMgr(0)
    , fErrorReporter(errReporter)
    , fReaderMgr(0)
    , fScanner(0)
{
}

// -----------------------------------------------------------------------
//  Notification that lazy data has been deleted
// -----------------------------------------------------------------------
void XMLValidator::reinitMsgMutex()
{
    delete sMsgMutex;
    sMsgMutex = 0;
}

// -----------------------------------------------------------------------
//  Reinitialise the message loader
// -----------------------------------------------------------------------
void XMLValidator::reinitMsgLoader()
{
	delete sMsgLoader;
	sMsgLoader = 0;
}

XERCES_CPP_NAMESPACE_END
