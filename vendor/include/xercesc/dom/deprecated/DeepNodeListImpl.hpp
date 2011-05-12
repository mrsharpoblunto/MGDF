#ifndef DeepNodeListImpl_HEADER_GUARD_
#define DeepNodeListImpl_HEADER_GUARD_

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
 * $Id: DeepNodeListImpl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
#include "NodeListImpl.hpp"
#include "DOMString.hpp"
#include "NodeImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NodeImpl;
class NodeVector;


class DEPRECATED_DOM_EXPORT DeepNodeListImpl: public NodeListImpl {
private:
    NodeImpl            *rootNode;
    DOMString           tagName;
    bool                matchAll;
    int                 changes;
    NodeVector          *nodes;

    //DOM Level 2
    DOMString		namespaceURI;
    bool		matchAllURI;
    bool                matchURIandTagname; //match both namespaceURI and tagName

public:
                        DeepNodeListImpl(NodeImpl *rootNode, const DOMString &tagName);
                        DeepNodeListImpl(NodeImpl *rootNode,	//DOM Level 2
			    const DOMString &namespaceURI, const DOMString &localName);
    virtual             ~DeepNodeListImpl();
    virtual unsigned int getLength();
    virtual NodeImpl    *item(unsigned int index);

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitDeepNodeListImpl();

private:
    virtual NodeImpl    *nextMatchingElementAfter(NodeImpl *current);
    virtual void        unreferenced();
};

XERCES_CPP_NAMESPACE_END

#endif
