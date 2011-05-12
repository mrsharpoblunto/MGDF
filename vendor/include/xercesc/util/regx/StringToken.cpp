/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: StringToken.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/StringToken.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Token: Constructors and Destructors
// ---------------------------------------------------------------------------
StringToken::StringToken(const unsigned short tokType,
                         const XMLCh* const literal,
                         const int refNo,
                         MemoryManager* const manager)
    : Token(tokType, manager)
    , fRefNo(refNo)
    , fString(XMLString::replicate(literal, manager))    
    , fMemoryManager(manager)
{

}


StringToken::~StringToken() {

	fMemoryManager->deallocate(fString);//delete [] fString;
}

XERCES_CPP_NAMESPACE_END

/**
  *	End of file StringToken.cpp
  */
