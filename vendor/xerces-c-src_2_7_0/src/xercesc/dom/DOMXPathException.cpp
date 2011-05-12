/*
 * Copyright 2001-2002,2004 The Apache Software Foundation.
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


#include "DOMXPathException.hpp"

XERCES_CPP_NAMESPACE_BEGIN


DOMXPathException::DOMXPathException()
: code((ExceptionCode) 0)
, msg(0)
{
}


DOMXPathException::DOMXPathException(short exCode, const XMLCh *message)
: code((ExceptionCode) exCode)
, msg(message)
{
}


DOMXPathException::DOMXPathException(const DOMXPathException &other)
: code(other.code)
, msg(other.msg)
{        
}


DOMXPathException::~DOMXPathException()
{
}

XERCES_CPP_NAMESPACE_END

