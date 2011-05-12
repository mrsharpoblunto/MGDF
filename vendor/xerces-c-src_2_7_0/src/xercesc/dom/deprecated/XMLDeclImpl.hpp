#ifndef XMLDeclImpl_HEADER_GUARD_
#define XMLDeclImpl_HEADER_GUARD_

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
 * $Id: XMLDeclImpl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

//
//  This file is part of the internal implementation of the C++ XML DOM.
//  It should NOT be included or used directly by application programs.
//
//  Applications should include the file <xercesc/dom/deprecated/DOM.hpp> for the entire
//  DOM API, or DOM_*.hpp for individual DOM classes, where the class
//  name is substituded for the *.
//

#include <xercesc/util/XercesDefs.hpp>
#include "ChildNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class DOMString;

class DEPRECATED_DOM_EXPORT XMLDeclImpl: public ChildNode {

private:
    // -----------------------------------------------------------------------
    //  Private data types
    // -----------------------------------------------------------------------
    DOMString version;
    DOMString encoding;
    DOMString standalone;

public:
    XMLDeclImpl(DocumentImpl *ownerDoc);
    XMLDeclImpl(DocumentImpl *ownerDoc, const DOMString& version,
                    const DOMString& encoding, const DOMString& standalone);
    XMLDeclImpl(const XMLDeclImpl &other, bool deep=false);
    virtual ~XMLDeclImpl();

    virtual NodeImpl * cloneNode(bool deep);
    virtual DOMString getNodeName();
    virtual short getNodeType();


    virtual DOMString getVersion() const;
    virtual DOMString getEncoding() const;
    virtual DOMString getStandalone() const;

    virtual void setVersion(const DOMString& data);
    virtual void setEncoding(const DOMString& data);
    virtual void setStandalone(const DOMString& data);

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitXMLDeclImpl();

};

XERCES_CPP_NAMESPACE_END

#endif
