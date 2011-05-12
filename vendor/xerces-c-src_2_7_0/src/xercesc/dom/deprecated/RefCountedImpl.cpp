/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: RefCountedImpl.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "RefCountedImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


RefCountedImpl::RefCountedImpl()
{
    nodeRefCount = 0;
};



RefCountedImpl::~RefCountedImpl()
{
};





void RefCountedImpl::addRef(RefCountedImpl *thisNode)
{
    if (thisNode)
    {
        if (++thisNode->nodeRefCount == 1)
        {
            thisNode->referenced();
        }
    }
};


void RefCountedImpl::referenced()
{
};



void RefCountedImpl::removeRef(RefCountedImpl *thisNode)
{
    if (thisNode)
    {
        if (--thisNode->nodeRefCount == 0)
        {
            thisNode->unreferenced();
        }
    }
};


void RefCountedImpl::unreferenced()
{
};


XERCES_CPP_NAMESPACE_END



