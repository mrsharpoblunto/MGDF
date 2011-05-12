/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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
 * $Id: CMUnaryOp.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/validators/common/CMStateSet.hpp>
#include <xercesc/validators/common/CMUnaryOp.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  CMUnaryOp: Constructors and Destructor
// ---------------------------------------------------------------------------
CMUnaryOp::CMUnaryOp( const ContentSpecNode::NodeTypes type
                    ,       CMNode* const              nodeToAdopt
                    ,       MemoryManager* const       manager) :
    CMNode(type, manager)
    , fChild(nodeToAdopt)
{
    // Insure that its one of the types we require
    if ((type != ContentSpecNode::ZeroOrOne)
    &&  (type != ContentSpecNode::ZeroOrMore)
    &&  (type != ContentSpecNode::OneOrMore))
    {
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::CM_UnaryOpHadBinType, manager);
    }
}

CMUnaryOp::~CMUnaryOp()
{
    delete fChild;
}


// ---------------------------------------------------------------------------
//  CMUnaryOp: Getter methods
// ---------------------------------------------------------------------------
const CMNode* CMUnaryOp::getChild() const
{
    return fChild;
}

CMNode* CMUnaryOp::getChild()
{
    return fChild;
}


// ---------------------------------------------------------------------------
//  CMUnaryOp: Implementation of the public CMNode virtual interface
// ---------------------------------------------------------------------------
bool CMUnaryOp::isNullable() const
{
    if (getType() == ContentSpecNode::OneOrMore)
        return fChild->isNullable();
    else
        return true;
}


// ---------------------------------------------------------------------------
//  CMUnaryOp: Implementation of the protected CMNode virtual interface
// ---------------------------------------------------------------------------
void CMUnaryOp::calcFirstPos(CMStateSet& toSet) const
{
    // Its just based on our child node's first pos
    toSet = fChild->getFirstPos();
}

void CMUnaryOp::calcLastPos(CMStateSet& toSet) const
{
    // Its just based on our child node's last pos
    toSet = fChild->getLastPos();
}

XERCES_CPP_NAMESPACE_END
