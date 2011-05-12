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
 * $Id: XSDElementNSImpl.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/dom/DOMException.hpp>

#include "DOMDocumentImpl.hpp"
#include "XSDElementNSImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


XSDElementNSImpl::XSDElementNSImpl(DOMDocument *ownerDoc, const XMLCh *nam) :
    DOMElementNSImpl(ownerDoc, nam)
    , fLineNo(0)
    , fColumnNo(0)
{
}

//Introduced in DOM Level 2
XSDElementNSImpl::XSDElementNSImpl(DOMDocument *ownerDoc,
                                   const XMLCh *namespaceURI,
                                   const XMLCh *qualifiedName,
                                   const XMLSSize_t lineNo,
                                   const XMLSSize_t columnNo) :
    DOMElementNSImpl(ownerDoc, namespaceURI, qualifiedName)
    , fLineNo(lineNo)
    , fColumnNo(columnNo)
{
}

XSDElementNSImpl::XSDElementNSImpl(const XSDElementNSImpl &other, bool deep) :
    DOMElementNSImpl(other, deep)
{
    this->fLineNo = other.fLineNo;
    this->fColumnNo =other.fColumnNo;
}

DOMNode * XSDElementNSImpl::cloneNode(bool deep) const {
    DOMNode* newNode = new (getOwnerDocument()) XSDElementNSImpl(*this, deep);
    fNode.callUserDataHandlers(DOMUserDataHandler::NODE_CLONED, this, newNode);
    return newNode;
}


XERCES_CPP_NAMESPACE_END

