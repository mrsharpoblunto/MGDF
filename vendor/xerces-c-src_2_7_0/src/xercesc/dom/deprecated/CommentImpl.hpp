#ifndef CommentImpl_HEADER_GUARD_
#define CommentImpl_HEADER_GUARD_

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
 * $Id: CommentImpl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
#include "CharacterDataImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class DEPRECATED_DOM_EXPORT CommentImpl: public CharacterDataImpl {
public:
        CommentImpl(DocumentImpl *, const DOMString &);
        CommentImpl(const CommentImpl &other, bool deep);
        virtual ~CommentImpl();
        virtual NodeImpl * cloneNode(bool deep);
    virtual DOMString getNodeName();
    virtual short getNodeType();

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitCommentImpl();
};

XERCES_CPP_NAMESPACE_END

#endif

