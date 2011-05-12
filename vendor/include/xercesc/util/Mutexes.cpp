/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: Mutexes.cpp 191054 2005-06-17 02:56:35Z jberry $
 */



// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/Mutexes.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  XMLMutex: Constructors and Destructor
// ---------------------------------------------------------------------------
XMLMutex::XMLMutex(MemoryManager* const manager) :

    fHandle(0)
{
    // Ask the per-platform driver to make us a mutex
    fHandle = XMLPlatformUtils::makeMutex(manager);
}


XMLMutex::~XMLMutex()
{
    if (fHandle)
    {
        XMLPlatformUtils::closeMutex(fHandle);
        fHandle = 0;
    }
}


// ---------------------------------------------------------------------------
//  XMLMutex: Lock control methods
// ---------------------------------------------------------------------------
void XMLMutex::lock()
{
    XMLPlatformUtils::lockMutex(fHandle);
}

void XMLMutex::unlock()
{
    XMLPlatformUtils::unlockMutex(fHandle);
}



// ---------------------------------------------------------------------------
//  XMLMutexLock: Constructors and Destructor
// ---------------------------------------------------------------------------
XMLMutexLock::XMLMutexLock(XMLMutex* const toLock) :

    fToLock(toLock)
{
    fToLock->lock();
}


XMLMutexLock::~XMLMutexLock()
{
    fToLock->unlock();
}

XERCES_CPP_NAMESPACE_END
