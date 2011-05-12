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
 * $Id: CDATASectionImpl.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "CDATASectionImpl.hpp"
#include "DOM_Node.hpp"
#include "DocumentImpl.hpp"
#include "DStringPool.hpp"

XERCES_CPP_NAMESPACE_BEGIN


static DOMString *gcdata_section = 0;   // will be lazily initialized
static XMLRegisterCleanup gcdata_sectionCleanup;

CDATASectionImpl::CDATASectionImpl(DocumentImpl *ownerDoc,
                                   const DOMString &dat)
    : TextImpl(ownerDoc, dat)
{
};


CDATASectionImpl::CDATASectionImpl(const CDATASectionImpl &other, bool deep)
    : TextImpl(other, deep)
{
};


CDATASectionImpl::~CDATASectionImpl()
{
};


NodeImpl  *CDATASectionImpl::cloneNode(bool deep)
{
    return new (getOwnerDocument()->getMemoryManager()) CDATASectionImpl(*this, deep);
};


DOMString CDATASectionImpl::getNodeName() {

    return DStringPool::getStaticString("#cdata-section"
                                       , &gcdata_section
                                       , reinitCDATASectionImpl
                                       , gcdata_sectionCleanup);
};


short CDATASectionImpl::getNodeType() {
    return DOM_Node::CDATA_SECTION_NODE;
};


bool CDATASectionImpl::isCDATASectionImpl()
{
    return true;
};

// -----------------------------------------------------------------------
//  Notification that lazy data has been deleted
// -----------------------------------------------------------------------
void CDATASectionImpl::reinitCDATASectionImpl() {
	delete gcdata_section;
	gcdata_section = 0;
}

XERCES_CPP_NAMESPACE_END

