/*
 * Copyright 2003,2004 The Apache Software Foundation.
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
 * $Id: MemoryManagerArrayImpl.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/internal/MemoryManagerArrayImpl.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

void* MemoryManagerArrayImpl::allocate(size_t size)
{
    
    void* memptr;
    try {
        //return ::operator new[](size);
        //return new char[size];
        memptr = new char[size];
    }
    catch(...) {
        throw OutOfMemoryException();
    }
    if (memptr != NULL) {
        return memptr;
    }
    throw OutOfMemoryException();
}

void MemoryManagerArrayImpl::deallocate(void* p)
{
    //::operator delete[](p);
	delete [] (char*)p;
}

XERCES_CPP_NAMESPACE_END

