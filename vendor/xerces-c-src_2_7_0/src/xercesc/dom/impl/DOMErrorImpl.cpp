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
 * $Id: DOMErrorImpl.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

#include "DOMErrorImpl.hpp"
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMLocator.hpp>

XERCES_CPP_NAMESPACE_BEGIN


// ---------------------------------------------------------------------------
//  DOMErrorImpl: Constructors and Destructor
// ---------------------------------------------------------------------------
DOMErrorImpl::DOMErrorImpl(const short severity) :
fAdoptLocation(false)
, fSeverity(severity)
, fMessage(0)
, fLocation(0)
, fType(0)
, fRelatedData(0)
{
}

DOMErrorImpl::DOMErrorImpl(const short severity,
                           const XMLCh* const message,
                           DOMLocator* const location) :
fAdoptLocation(false)
, fSeverity(severity)
, fMessage(message)
, fLocation(location)
, fType(0)
, fRelatedData(0)
{
}

DOMErrorImpl::DOMErrorImpl(const short severity,
                           const XMLCh* type,
                           const XMLCh* message,
                           void* relatedData) :
fAdoptLocation(false)
, fSeverity(severity)
, fMessage(message)
, fLocation(0)
, fType(type)
, fRelatedData(relatedData)
{

}

DOMErrorImpl::~DOMErrorImpl()
{
    if (fAdoptLocation)
        delete fLocation;
}

// ---------------------------------------------------------------------------
//  DOMErrorImpl: Setter methods
// ---------------------------------------------------------------------------
void DOMErrorImpl::setLocation(DOMLocator* const location)
{
    if (fAdoptLocation)
        delete fLocation;

    fLocation = location;
}

void DOMErrorImpl::setRelatedException(void*) const
{
    throw DOMException(DOMException::NOT_SUPPORTED_ERR, 0);
}

XERCES_CPP_NAMESPACE_END

