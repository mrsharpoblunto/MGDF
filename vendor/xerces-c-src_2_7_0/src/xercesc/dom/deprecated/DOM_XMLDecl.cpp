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
 * $Id: DOM_XMLDecl.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#include "DOM_XMLDecl.hpp"
#include "XMLDeclImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


DOM_XMLDecl::DOM_XMLDecl() :
        DOM_Node(null)
{
};


DOM_XMLDecl::DOM_XMLDecl(const DOM_XMLDecl &other)
: DOM_Node(other)
{
};

DOM_XMLDecl::DOM_XMLDecl(XMLDeclImpl *impl) :
        DOM_Node(impl)
{
};

DOM_XMLDecl::~DOM_XMLDecl()
{
};

DOM_XMLDecl & DOM_XMLDecl::operator = (const DOM_XMLDecl &other)
{
    return (DOM_XMLDecl &) DOM_Node::operator = (other);
};


DOM_XMLDecl & DOM_XMLDecl::operator = (const DOM_NullPtr *other)
{
    return (DOM_XMLDecl &) DOM_Node::operator = (other);
};

DOMString DOM_XMLDecl::getVersion() const
{
    return DOMString( ((XMLDeclImpl*)fImpl)->getVersion());
}

DOMString DOM_XMLDecl::getEncoding() const
{
    return DOMString( ((XMLDeclImpl*)fImpl)->getEncoding());
}

DOMString DOM_XMLDecl::getStandalone() const
{
    return DOMString( ((XMLDeclImpl*)fImpl)->getStandalone());
}

XERCES_CPP_NAMESPACE_END

