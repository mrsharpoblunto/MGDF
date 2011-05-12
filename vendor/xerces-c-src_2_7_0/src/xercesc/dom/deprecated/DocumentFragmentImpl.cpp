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
 * $Id: DocumentFragmentImpl.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "DocumentFragmentImpl.hpp"
#include "DOM_Node.hpp"
#include "DOM_DOMException.hpp"
#include "DStringPool.hpp"
#include "DocumentImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN



static DOMString *nam = 0;   // Will be lazily initialized to "#document-fragment"
static XMLRegisterCleanup namCleanup;

DocumentFragmentImpl::DocumentFragmentImpl(DocumentImpl *masterDoc)
    : ParentNode(masterDoc)
{
};


DocumentFragmentImpl::DocumentFragmentImpl(const DocumentFragmentImpl &other,
                                           bool deep)
    : ParentNode(other)
{
    if (deep)
        cloneChildren(other);
};


DocumentFragmentImpl::~DocumentFragmentImpl()
{
};



NodeImpl *DocumentFragmentImpl::cloneNode(bool deep)
{
    return new (getOwnerDocument()->getMemoryManager()) DocumentFragmentImpl(*this, deep);
};


DOMString DocumentFragmentImpl::getNodeName() {
    return DStringPool::getStaticString("#document-fragment"
                                       , &nam
                                       , reinitDocumentFragmentImpl
                                       , namCleanup);
}


short DocumentFragmentImpl::getNodeType() {
    return DOM_Node::DOCUMENT_FRAGMENT_NODE;
};


bool DocumentFragmentImpl::isDocumentFragmentImpl()
{
        return true;
};


// -----------------------------------------------------------------------
//  Notification that lazy data has been deleted
// -----------------------------------------------------------------------
void DocumentFragmentImpl::reinitDocumentFragmentImpl() {
	delete nam;
	nam = 0;
}

XERCES_CPP_NAMESPACE_END

