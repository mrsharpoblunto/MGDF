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
 * $Id: DOM_ProcessingInstruction.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "DOM_ProcessingInstruction.hpp"
#include "ProcessingInstructionImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


DOM_ProcessingInstruction::DOM_ProcessingInstruction()
: DOM_Node(null)
{
};


DOM_ProcessingInstruction::DOM_ProcessingInstruction(
                                               const DOM_ProcessingInstruction & other)
: DOM_Node(other)
{
};


DOM_ProcessingInstruction::DOM_ProcessingInstruction(
                                               ProcessingInstructionImpl *impl)
:  DOM_Node(impl)
{
};


DOM_ProcessingInstruction::~DOM_ProcessingInstruction()
{
};


DOM_ProcessingInstruction & DOM_ProcessingInstruction::operator = (
                                        const DOM_ProcessingInstruction & other)
{
    return (DOM_ProcessingInstruction &) DOM_Node::operator = (other);
};


DOM_ProcessingInstruction & DOM_ProcessingInstruction::operator = (const DOM_NullPtr *other)
{
    return (DOM_ProcessingInstruction &) DOM_Node::operator = (other);
};



DOMString DOM_ProcessingInstruction::getTarget() const
{
    return ((ProcessingInstructionImpl *)fImpl)->getTarget().clone();
};


DOMString DOM_ProcessingInstruction::getData() const
{
    return ((ProcessingInstructionImpl *)fImpl)->getData().clone();
};


void      DOM_ProcessingInstruction::setData(const DOMString &data)
{
    ((ProcessingInstructionImpl *)fImpl)->setData(data.clone());
};


XERCES_CPP_NAMESPACE_END

