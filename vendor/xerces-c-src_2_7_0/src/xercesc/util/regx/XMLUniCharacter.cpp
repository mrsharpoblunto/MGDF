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
 * $Id: XMLUniCharacter.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/XMLUniCharacter.hpp>

#if defined (XML_USE_ICU_TRANSCODER) || defined (XML_USE_UNICONV390_TRANSCODER)
   #include <unicode/uchar.h>
#else
   #include <xercesc/util/regx/UniCharTable.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  XMLUniCharacter: Public static methods
// ---------------------------------------------------------------------------
unsigned short XMLUniCharacter::getType(const XMLCh ch) {

#if defined (XML_USE_ICU_TRANSCODER) || defined (XML_USE_UNICONV390_TRANSCODER)
	return (unsigned short) u_charType(ch);
#else
	return (unsigned short) fgUniCharsTable[ch];
#endif
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file XMLUniCharacter.cpp
  */

