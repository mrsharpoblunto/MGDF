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
 * $Id: DOM_Text.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "DOM_Text.hpp"
#include "TextImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


DOM_Text::DOM_Text()
: DOM_CharacterData(null)
{
};


DOM_Text::DOM_Text(const DOM_Text & other)
: DOM_CharacterData(other)
{
};


DOM_Text::DOM_Text(TextImpl *impl) :
        DOM_CharacterData(impl)
{
};


DOM_Text::~DOM_Text()
{
};


DOM_Text & DOM_Text::operator = (const DOM_Text & other)
{
    return (DOM_Text &) DOM_CharacterData::operator = (other);
};


DOM_Text & DOM_Text::operator = (const DOM_NullPtr *other)
{
    return (DOM_Text &) DOM_Node::operator = (other);
};


DOM_Text DOM_Text::splitText(unsigned int offset)
{
        return DOM_Text(((TextImpl *)fImpl)->splitText(offset));
};


bool DOM_Text::isIgnorableWhitespace()
{
        return ((TextImpl *)fImpl)->isIgnorableWhitespace();
};

XERCES_CPP_NAMESPACE_END

