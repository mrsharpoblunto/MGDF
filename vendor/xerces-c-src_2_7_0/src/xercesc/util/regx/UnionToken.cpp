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
 * $Id: UnionToken.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/UnionToken.hpp>
#include <xercesc/framework/XMLBuffer.hpp>
#include <xercesc/util/regx/RegxUtil.hpp>
#include <xercesc/util/regx/TokenFactory.hpp>
#include <xercesc/util/regx/StringToken.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Static member data initialization
// ---------------------------------------------------------------------------
const unsigned short UnionToken::INITIALSIZE = 8;

// ---------------------------------------------------------------------------
//  UnionToken: Constructors and Destructors
// ---------------------------------------------------------------------------
UnionToken::UnionToken(const unsigned short tokType, MemoryManager* const manager)
    : Token(tokType, manager)
    , fChildren(0)
{

}

UnionToken::~UnionToken() {

    delete fChildren;
}


// ---------------------------------------------------------------------------
//  UnionToken: Children manipulation methods
// ---------------------------------------------------------------------------
void UnionToken::addChild(Token* const child, TokenFactory* const tokFactory) {

    if (child == 0)
        return;

    if (fChildren == 0)
        fChildren = new (tokFactory->getMemoryManager()) RefVectorOf<Token>(INITIALSIZE, false, tokFactory->getMemoryManager());

    if (getTokenType() == T_UNION) {

        fChildren->addElement(child);
        return;
    }

    unsigned short childType = child->getTokenType();
    unsigned int   childSize = child->size();

    if (childType == T_CONCAT) {

        for (unsigned int i = 0; i < childSize; i++) {

            addChild(child->getChild(i), tokFactory);
        }

        return;
    }

    unsigned int childrenSize = fChildren->size();
    if (childrenSize == 0) {

        fChildren->addElement(child);
        return;
    }

    Token* previousTok = fChildren->elementAt(childrenSize - 1);
    unsigned short previousType = previousTok->getTokenType();

    if (!((previousType == T_CHAR || previousType == T_STRING)
          && (childType == T_CHAR || childType == T_STRING))) {

        fChildren->addElement(child);
        return;
    }

    // Continue
    XMLBuffer stringBuf(1023, tokFactory->getMemoryManager());

    if (previousType == T_CHAR) {

        XMLInt32 ch = previousTok->getChar();

        if (ch >= 0x10000) {

            XMLCh* chSurrogate = RegxUtil::decomposeToSurrogates(ch, tokFactory->getMemoryManager());
            stringBuf.append(chSurrogate);
            tokFactory->getMemoryManager()->deallocate(chSurrogate);//delete [] chSurrogate;
        }
        else {
            stringBuf.append((XMLCh) ch);
        }

        previousTok = tokFactory->createString(0);
        fChildren->setElementAt(previousTok, childrenSize - 1);
    }
    else {
        stringBuf.append(previousTok->getString());
    }

    if (childType == T_CHAR) {

        XMLInt32 ch = child->getChar();

        if (ch >= 0x10000) {

            XMLCh* chSurrogate = RegxUtil::decomposeToSurrogates(ch, tokFactory->getMemoryManager());
            stringBuf.append(chSurrogate);
            tokFactory->getMemoryManager()->deallocate(chSurrogate);//delete [] chSurrogate;
        }
        else {
            stringBuf.append((XMLCh) ch);
        }
    }
    else {
        stringBuf.append(child->getString());
    }

    ((StringToken*) previousTok)->setString(stringBuf.getRawBuffer());
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file UnionToken.cpp
  */
