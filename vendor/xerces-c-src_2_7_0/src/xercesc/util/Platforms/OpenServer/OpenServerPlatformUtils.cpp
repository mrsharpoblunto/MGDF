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
 * $Id: OpenServerPlatformUtils.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

#if !defined (APP_NO_THREADS)
#include     <pthread.h>
#endif


#include    <unistd.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <errno.h>
#include    <libgen.h>
#include    <sys/time.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <xercesc/util/PlatformUtils.hpp>
#include    <xercesc/util/RuntimeException.hpp>
#include    <xercesc/util/Janitor.hpp>
#include    <xercesc/util/Mutexes.hpp>
#include    <xercesc/util/XMLHolder.hpp>
#include    <xercesc/util/XMLString.hpp>
#include    <xercesc/util/XMLUniDefs.hpp>
#include    <xercesc/util/XMLUni.hpp>
#include    <xercesc/util/PanicHandler.hpp>
#include    <xercesc/util/OutOfMemoryException.hpp>

#if defined (XML_USE_ICU_TRANSCODER)
#   include <xercesc/util/Transcoders/ICU/ICUTransService.hpp>
#else
//  Same as -DXML_USE_NATIVE_TRANSCODER
#   include <xercesc/util/Transcoders/Iconv/IconvTransService.hpp>
#endif


#if defined(XML_USE_ICU_MESSAGELOADER)
#   include <xercesc/util/MsgLoaders/ICU/ICUMsgLoader.hpp>
#elif defined (XML_USE_ICONV_MESSAGELOADER)
#   include <xercesc/util/MsgLoaders/MsgCatalog/MsgCatalogLoader.hpp>
#else
//  Same as -DXML_USE_INMEM_MESSAGELOADER
#   include <xercesc/util/MsgLoaders/InMemory/InMemMsgLoader.hpp>
#endif


#if defined (XML_USE_NETACCESSOR_LIBWWW)
#   include <xercesc/util/NetAccessors/libWWW/LibWWWNetAccessor.hpp>
#else
//  Same as -DXML_USE_NETACCESSOR_SOCKET
#   include <xercesc/util/NetAccessors/Socket/SocketNetAccessor.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Private Static Methods
// ---------------------------------------------------------------------------

//
//  This method is called by the platform independent part of this class
//  when client code asks to have one of the supported message sets loaded.
//
XMLMsgLoader* XMLPlatformUtils::loadAMsgSet(const XMLCh* const msgDomain)
{
    XMLMsgLoader* retVal = 0;
    try
    {
#if defined (XML_USE_ICU_MESSAGELOADER)
        retVal = new (fgMemoryManager) ICUMsgLoader(msgDomain);
#elif defined (XML_USE_ICONV_MESSAGELOADER)
        retVal = new (fgMemoryManager) MsgCatalogLoader(msgDomain);
#else
        retVal = new (fgMemoryManager) InMemMsgLoader(msgDomain);
#endif
    }
    catch(const OutOfMemoryException&)
    {
        throw;
    }
    catch(...)
    {
        panic(PanicHandler::Panic_CantLoadMsgDomain);
    }
    return retVal;
}


XMLNetAccessor* XMLPlatformUtils::makeNetAccessor()
{
#if defined (XML_USE_NETACCESSOR_LIBWWW)
    return new (fgMemoryManager) LibWWWNetAccessor();
#else
    return new (fgMemoryManager) SocketNetAccessor();
#endif
}


//
//  This method is called very early in the bootstrapping process. This guy
//  must create a transcoding service and return it. It cannot use any string
//  methods, any transcoding services, throw any exceptions, etc... It just
//  makes a transcoding service and returns it, or returns zero on failure.
//

XMLTransService* XMLPlatformUtils::makeTransService()
{
#if defined (XML_USE_ICU_TRANSCODER)
	return new (fgMemoryManager) ICUTransService;
#else
	return new (fgMemoryManager) IconvTransService;
#endif
}


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: The panic method
// ---------------------------------------------------------------------------
void XMLPlatformUtils::panic(const PanicHandler::PanicReasons reason)
{
    fgUserPanicHandler? fgUserPanicHandler->panic(reason) : fgDefaultPanicHandler->panic(reason);	
}

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: File Methods
// ---------------------------------------------------------------------------
unsigned int XMLPlatformUtils::curFilePos(FileHandle theFile
                                          , MemoryManager* const manager)
{
    // Get the current position
    int curPos = ftell((FILE*)theFile);
    if (curPos == -1)
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotGetSize, manager);

    return (unsigned int)curPos;
}

void XMLPlatformUtils::closeFile(FileHandle theFile
                                 , MemoryManager* const manager)
{
    if (fclose((FILE*)theFile))
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotCloseFile, manager);
}

unsigned int XMLPlatformUtils::fileSize(FileHandle theFile
                                        , MemoryManager* const manager)
{
    // Get the current position
    long int curPos = ftell((FILE*)theFile);
    if (curPos == -1)
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotGetCurPos, manager);

    // Seek to the end and save that value for return
	if (fseek((FILE*)theFile, 0, SEEK_END))
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotSeekToEnd, manager);

    long int retVal = ftell((FILE*)theFile);
    if (retVal == -1)
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotSeekToEnd, manager);

    // And put the pointer back
    if (fseek((FILE*)theFile, curPos, SEEK_SET))
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotSeekToPos, manager);

    return (unsigned int)retVal;
}

FileHandle XMLPlatformUtils::openFile(const XMLCh* const fileName
                                      , MemoryManager* const manager)
{
    const char* tmpFileName = XMLString::transcode(fileName, manager);
    ArrayJanitor<char> janText((char*)tmpFileName, manager);
    FileHandle retVal = (FILE*)fopen(tmpFileName , "rb");

    if (retVal == NULL)
        return 0;

    return retVal;
}

FileHandle XMLPlatformUtils::openFile(const char* const fileName
                                      , MemoryManager* const manager)
{
    FileHandle retVal = (FILE*)fopen(fileName, "rb");

    if (retVal == NULL)
        return 0;

    return retVal;
}

FileHandle XMLPlatformUtils::openFileToWrite(const XMLCh* const fileName
                                             , MemoryManager* const manager)
{
    const char* tmpFileName = XMLString::transcode(fileName, manager);
    ArrayJanitor<char> janText((char*)tmpFileName, manager);
    return fopen( tmpFileName , "wb" );
}

FileHandle XMLPlatformUtils::openFileToWrite(const char* const fileName
                                             , MemoryManager* const manager)
{
    return fopen( fileName , "wb" );
}

FileHandle XMLPlatformUtils::openStdInHandle(MemoryManager* const manager)
{
	return (FileHandle)fdopen(dup(0), "rb");
}

unsigned int XMLPlatformUtils::readFileBuffer(FileHandle            theFile,
											  const unsigned int    toRead,
											  XMLByte* const        toFill
                                              , MemoryManager* const manager)
{
    size_t noOfItemsRead = fread((void*)toFill, 1, toRead, (FILE*)theFile);

    if(ferror((FILE*)theFile))
    {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotReadFromFile, manager);
    }

    return (unsigned int)noOfItemsRead;
}

void
XMLPlatformUtils::writeBufferToFile( FileHandle     const  theFile
                                   , long                  toWrite
                                   , const XMLByte* const  toFlush
                                   , MemoryManager* const  manager)                                   
{
    if (!theFile        ||
        (toWrite <= 0 ) ||
        !toFlush         )
        return;

    const XMLByte* tmpFlush = (const XMLByte*) toFlush;
    size_t bytesWritten = 0;

    while (true)
    {
        bytesWritten=fwrite(tmpFlush, sizeof(XMLByte), toWrite, (FILE*)theFile);

        if(ferror((FILE*)theFile))
        {
            ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotWriteToFile, manager);
        }

        if (bytesWritten < toWrite) //incomplete write
        {
            tmpFlush+=bytesWritten;
            toWrite-=bytesWritten;
            bytesWritten=0;
        }
        else
            return;
    }

    return;
}

void XMLPlatformUtils::resetFile(FileHandle theFile
                                 , MemoryManager* const manager)
{
    if (fseek((FILE*)theFile, 0, SEEK_SET))
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotResetFile, manager);
}

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: File system methods
// ---------------------------------------------------------------------------
XMLCh* XMLPlatformUtils::getFullPath(const XMLCh* const srcPath,
                                     MemoryManager* const manager)
{
    //
    //  NOTE: The path provided has always already been opened successfully,
    //  so we know that its not some pathological freaky path. It comes in
    //  in native format, and goes out as Unicode always
    //
    char* newSrc = XMLString::transcode(srcPath, manager);
    ArrayJanitor<char> janText(newSrc, manager);

    // Use a local buffer that is big enough for the largest legal path
    // Without the *3 we get exceptions with gcc on OpenServer 5.0.5/6 when 
    // relative paths are passed in
    char *absPath = (char*) manager->allocate
    (
        (pathconf(newSrc, _PC_PATH_MAX)*3) * sizeof(char)
    );//new char[pathconf(newSrc, _PC_PATH_MAX)*3];
    ArrayJanitor<char> janText2(absPath, manager);
    // Get the absolute path
    char* retPath = realpath(newSrc, absPath);

    if (!retPath)
    {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotGetBasePathName, manager);
    }
    return XMLString::transcode(absPath, manager);
}

bool XMLPlatformUtils::isRelative(const XMLCh* const toCheck
                                  , MemoryManager* const manager)
{
    // Check for pathological case of empty path
    if (!toCheck[0])
        return false;

    //
    //  If it starts with a slash, then it cannot be relative. This covers
    //  both something like "\Test\File.xml" and an NT Lan type remote path
    //  that starts with a node like "\\MyNode\Test\File.xml".
    //
    if (toCheck[0] == XMLCh('/'))
        return false;

    // Else assume its a relative path
    return true;
}

XMLCh* XMLPlatformUtils::getCurrentDirectory(MemoryManager* const manager)
{
    char  dirBuf[PATH_MAX + 2];
    char  *curDir = getcwd(&dirBuf[0], PATH_MAX + 1);

    if (!curDir)
    {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotGetBasePathName, manager);
    }

    return XMLString::transcode(curDir, manager);
}

inline bool XMLPlatformUtils::isAnySlash(XMLCh c) 
{
    return ( chBackSlash == c || chForwardSlash == c);
}

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Timing Methods
// ---------------------------------------------------------------------------
unsigned long XMLPlatformUtils::getCurrentMillis()
{
	struct timeval   tp;
	if (gettimeofday(&tp, NULL) == -1)
		return 0;
    return (unsigned long)(tp.tv_sec * 1000 + tp.tv_usec / 1000);
}


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Methods for multi-threaded environment
// ---------------------------------------------------------------------------
#if !defined (APP_NO_THREADS)

typedef XMLHolder<pthread_mutex_t>  MutexHolderType;

static MutexHolderType* gAtomicOpMutex = 0;

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Platform init method
// ---------------------------------------------------------------------------
void XMLPlatformUtils::platformInit()
{
    //
    // The atomicOps mutex needs to be created early.
    // Normally, mutexes are created on first use, but there is a
    // circular dependency between compareAndExchange() and
    // mutex creation that must be broken.
	gAtomicOpMutex = new (fgMemoryManager) MutexHolderType;

	if (pthread_mutex_init(&gAtomicOpMutex->fInstance, NULL))
	{
		delete gAtomicOpMutex;
		gAtomicOpMutex = 0;
		panic(PanicHandler::Panic_SystemInit);
	}
}

// -----------------------------------------------------------------------
//  Mutex- and atomic operation methods
// -----------------------------------------------------------------------
class  RecursiveMutex : public XMemory
{
public:
	pthread_mutex_t   mutex;
	int               recursionCount;
	pthread_t         tid;
    MemoryManager* const    fMemoryManager;

	RecursiveMutex(MemoryManager* manager) :
        mutex(),
        recursionCount(0),
        tid(0),
        fMemoryManager(manager) {
		if (pthread_mutex_init(&mutex, NULL))
			XMLPlatformUtils::panic(PanicHandler::Panic_MutexErr);
	}

	~RecursiveMutex() {
		if (pthread_mutex_destroy(&mutex))
			ThrowXMLwithMemMgr(XMLPlatformUtilsException,
					 XMLExcepts::Mutex_CouldNotDestroy, fMemoryManager);
	}

	void lock() {
		if (pthread_equal(tid, pthread_self()))
		{
			recursionCount++;
			return;
		}
		if (pthread_mutex_lock(&mutex) != 0)
			XMLPlatformUtils::panic(PanicHandler::Panic_MutexErr);
		tid = pthread_self();
		recursionCount = 1;
	}

	void unlock() {
		if (--recursionCount > 0)
			return;

		if (pthread_mutex_unlock(&mutex) != 0)
			XMLPlatformUtils::panic(PanicHandler::Panic_MutexErr);
		tid = 0;
	}
};

void* XMLPlatformUtils::makeMutex(MemoryManager* manager)
{
	return new (manager) RecursiveMutex(manager);
}

void XMLPlatformUtils::closeMutex(void* const mtxHandle)
{
	if (mtxHandle == NULL)
		return;

	RecursiveMutex *rm = (RecursiveMutex *)mtxHandle;
	delete rm;
}

void XMLPlatformUtils::lockMutex(void* const mtxHandle)
{
	if (mtxHandle == NULL)
		return;

	RecursiveMutex *rm = (RecursiveMutex *)mtxHandle;
	rm->lock();
}

void XMLPlatformUtils::unlockMutex(void* const mtxHandle)
{
	if (mtxHandle == NULL)
		return;

	RecursiveMutex *rm = (RecursiveMutex *)mtxHandle;
	rm->unlock();
}

// -----------------------------------------------------------------------
//  Miscellaneous synchronization methods
// -----------------------------------------------------------------------
// Atomic system calls in OpenServe is restricted to kernel libraries.
// To make operations thread safe, we implement static mutex and lock
// the atomic operations.

void* XMLPlatformUtils::compareAndSwap(void**              toFill,
									   const void* const   newValue,
									   const void* const   toCompare)
{
	if (pthread_mutex_lock(&gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	void *retVal = *toFill;
	if (*toFill == toCompare)
		*toFill = (void *)newValue;

	if (pthread_mutex_unlock(&gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	return retVal;
}

int XMLPlatformUtils::atomicIncrement(int &location)
{
	if (pthread_mutex_lock(&gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	int tmp = ++location;

	if (pthread_mutex_unlock(&gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	return tmp;
}

int XMLPlatformUtils::atomicDecrement(int &location)
{
	if (pthread_mutex_lock(&gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	int tmp = --location;

	if (pthread_mutex_unlock( &gAtomicOpMutex->fInstance))
		panic(PanicHandler::Panic_SynchronizationErr);

	return tmp;
}

#else // #if !defined (APP_NO_THREADS)


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Methods for single-threaded environment
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Platform init method
// ---------------------------------------------------------------------------
void XMLPlatformUtils::platformInit()
{
}

// -----------------------------------------------------------------------
//  Mutex- and atomic operation methods
// -----------------------------------------------------------------------
void* XMLPlatformUtils::makeMutex(MemoryManager*)
{
	return 0;
}

void XMLPlatformUtils::closeMutex(void* const)
{
}

void XMLPlatformUtils::lockMutex(void* const)
{
}

void XMLPlatformUtils::unlockMutex(void* const)
{
}

void* XMLPlatformUtils::compareAndSwap(void**              toFill,
									   const void* const   newValue,
									   const void* const   toCompare)
{
    void *retVal = *toFill;
    if (*toFill == toCompare)
		*toFill = (void *)newValue;
    return retVal;
}

int XMLPlatformUtils::atomicIncrement(int &location)
{
    return ++location;
}

int XMLPlatformUtils::atomicDecrement(int &location)
{
    return --location;
}

#endif // APP_NO_THREADS


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Platform termination method
// ---------------------------------------------------------------------------
void XMLPlatformUtils::platformTerm()
{
#if !defined (APP_NO_THREADS)
	pthread_mutex_destroy(&gAtomicOpMutex->fInstance);
	delete gAtomicOpMutex;
	gAtomicOpMutex = 0;
#else
    // If not using threads, we don't have any termination requirements (yet)
#endif
}

#include <xercesc/util/LogicalPath.c>

XERCES_CPP_NAMESPACE_END

