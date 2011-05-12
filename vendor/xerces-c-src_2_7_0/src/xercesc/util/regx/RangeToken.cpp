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

/*
 * $Id: RangeToken.cpp 225535 2005-07-27 15:08:56Z cargilld $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <assert.h>

#include <xercesc/util/regx/RangeToken.hpp>
#include <xercesc/util/regx/TokenFactory.hpp>
#include <xercesc/util/IllegalArgumentException.hpp>

#if defined(XML_USE_ICU_TRANSCODER) || defined (XML_USE_UNICONV390_TRANSCODER)
#include <unicode/uchar.h>
#else
#include <xercesc/util/XMLUniDefs.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Static member data initialization
// ---------------------------------------------------------------------------
const int RangeToken::MAPSIZE = 256;
const unsigned int RangeToken::INITIALSIZE = 16;

// ---------------------------------------------------------------------------
//  RangeToken: Constructors and Destructors
// ---------------------------------------------------------------------------
RangeToken::RangeToken(const unsigned short tokType,
                       MemoryManager* const manager) 
    : Token(tokType, manager)
    , fSorted(false)
    , fCompacted(false)
    , fNonMapIndex(0)
    , fElemCount(0)
    , fMaxCount(INITIALSIZE)
    , fMap(0)
    , fRanges(0)
    , fCaseIToken(0)
    , fMemoryManager(manager)
{

}

RangeToken::~RangeToken() {

    fMemoryManager->deallocate(fMap);//delete [] fMap;
    fMemoryManager->deallocate(fRanges);//delete[] fRanges;
}


// ---------------------------------------------------------------------------
//  RangeToken: Getter methods
// ---------------------------------------------------------------------------
RangeToken* RangeToken::getCaseInsensitiveToken(TokenFactory* const tokFactory) {

    if (fCaseIToken == 0 && tokFactory) {

        bool isNRange = (getTokenType() == T_NRANGE) ? true : false;
        RangeToken* lwrToken = tokFactory->createRange(isNRange);

        for (unsigned int i = 0;  i < fElemCount - 1;  i += 2) {
            for (XMLInt32 ch = fRanges[i];  ch <= fRanges[i + 1];  ++ch) {
#if defined(XML_USE_ICU_TRANSCODER) || defined (XML_USE_UNICONV390_TRANSCODER)
                const XMLInt32  upperCh = u_toupper(ch);

                if (upperCh != ch)
                {
                    lwrToken->addRange(upperCh, upperCh);
                }

                const XMLInt32  lowerCh = u_tolower(ch);

                if (lowerCh != ch)
                {
                    lwrToken->addRange(lowerCh, lowerCh);
                }

                const XMLInt32  titleCh = u_totitle(ch);

                if (titleCh != ch && titleCh != upperCh)
                {
                    lwrToken->addRange(titleCh, titleCh);
                }
#else
                if (ch >= chLatin_A && ch <= chLatin_Z)
                {
                    ch += chLatin_a - chLatin_A;

                    lwrToken->addRange(ch, ch);
                }
                else if (ch >= chLatin_a && ch <= chLatin_z)
                {
                    ch -= chLatin_a - chLatin_A;

                    lwrToken->addRange(ch, ch);
                }
#endif
            }
        }

        lwrToken->mergeRanges(this);
        lwrToken->compactRanges();
        lwrToken->createMap();

        fCaseIToken = lwrToken;
    }

    return fCaseIToken;
}

// ---------------------------------------------------------------------------
//  RangeToken: Setter methods
// ---------------------------------------------------------------------------
void RangeToken::setRangeValues(XMLInt32* const rangeValues, const unsigned int count)
{
    if (fRanges) {

        if (fMap) {
            fMemoryManager->deallocate(fMap);//delete [] fMap;
            fMap = 0;
        }

        fElemCount = 0;
        fMemoryManager->deallocate(fRanges);//delete [] fRanges;
        fRanges = 0;
    }

    fElemCount = fMaxCount = count;
    fRanges = rangeValues;
}

// ---------------------------------------------------------------------------
//  RangeToken: Range manipulation methods
// ---------------------------------------------------------------------------
void RangeToken::addRange(const XMLInt32 start, const XMLInt32 end) {

    XMLInt32 val1, val2;

    fCaseIToken = 0;

    if (start <= end) {

        val1 = start;
        val2 = end;
    }
    else {

        val1 = end;
        val2 = start;
    }

    if (fRanges == 0) {

        fRanges = (XMLInt32*) fMemoryManager->allocate
        (
            fMaxCount * sizeof(XMLInt32)
        );//new XMLInt32[fMaxCount];
        fRanges[0] = val1;
        fRanges[1] = val2;
        fElemCount = 2;
        fSorted = true;
    }
    else {

        if (fRanges[fElemCount-1] + 1 == val1) {

            fRanges[fElemCount-1] = val2;
            return;
        }

        if (fElemCount + 2 >= fMaxCount) {
            expand(2);
        }

        if (fRanges[fElemCount-1] >= val1)
            fSorted = false;

        fRanges[fElemCount++] = val1;
        fRanges[fElemCount++] = val2;

        if (!fSorted) {
            sortRanges();
        }
    }
}

void RangeToken::sortRanges() {

    if (fSorted || fRanges == 0)
        return;

    for (int i = fElemCount - 4; i >= 0; i -= 2) {

        for (int j = 0; j <= i; j +=2) {

            if (fRanges[j] > fRanges[j + 2]
                || (fRanges[j]==fRanges[j+2] && fRanges[j+1] > fRanges[j+3])) {

                XMLInt32 tmpVal = fRanges[j+2];
                fRanges[j+2] = fRanges[j];
                fRanges[j] = tmpVal;
                tmpVal = fRanges[j+3];
                fRanges[j+3] = fRanges[j+1];
                fRanges[j+1] = tmpVal;
            }
        }
    }

    fSorted = true;
}

void RangeToken::compactRanges() {

    if (fCompacted || fRanges == 0 || fElemCount <= 2)
        return;

    unsigned int base = 0;
    unsigned int target = 0;

    while (target < fElemCount) {

        if (base != target) {

            fRanges[base] = fRanges[target++];
            fRanges[base+1] = fRanges[target++];
        }
        else
            target += 2;

        XMLInt32 baseEnd = fRanges[base + 1];

        while (target < fElemCount) {

            XMLInt32 startRange = fRanges[target];

            if (baseEnd + 1 < startRange)
                break;

            XMLInt32 endRange = fRanges[target + 1];

            if (baseEnd + 1 == startRange || baseEnd < endRange) {

                baseEnd = endRange;
                fRanges[base+1] = baseEnd;
                target += 2;
            }
            else if (baseEnd >= endRange) {
                target += 2;
            }
            else {
                ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_CompactRangesError, fMemoryManager);
            }
        } // inner while

        base += 2;
    }

    fElemCount = base;
    fCompacted = true;
}

void RangeToken::mergeRanges(const Token *const tok) {


    if (tok->getTokenType() != this->getTokenType())
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Regex_MergeRangesTypeMismatch, fMemoryManager);

    RangeToken* rangeTok = (RangeToken *) tok;

    if (rangeTok->fRanges == 0)
        return;

    fCaseIToken = 0;
    sortRanges();
    rangeTok->sortRanges();

    if (fRanges == 0) {

        fMaxCount = rangeTok->fMaxCount;
        fRanges = (XMLInt32*) fMemoryManager->allocate
        (
            fMaxCount * sizeof(XMLInt32)
        );//new XMLInt32[fMaxCount];
        for (unsigned int index = 0; index < rangeTok->fElemCount; index++) {
            fRanges[index] = rangeTok->fRanges[index];
        }

        fElemCount = rangeTok->fElemCount;
        fSorted = true;
        return;
    }

    unsigned int newMaxCount = (fElemCount + rangeTok->fElemCount >= fMaxCount)
                                 ? fMaxCount + rangeTok->fMaxCount : fMaxCount;
    XMLInt32* result = (XMLInt32*) fMemoryManager->allocate
    (
        newMaxCount * sizeof(XMLInt32)
    );//new XMLInt32[newMaxCount];

    for (unsigned int i=0, j=0, k=0; i < fElemCount || j < rangeTok->fElemCount;) {

        if (i >= fElemCount) {

            for (int count = 0; count < 2; count++) {
                result[k++] = rangeTok->fRanges[j++];
            }
        }
        else if (j >= rangeTok->fElemCount) {

            for (int count = 0; count < 2; count++) {
                result[k++] = fRanges[i++];
            }
        }
        else if (rangeTok->fRanges[j] < fRanges[i]
                 || (rangeTok->fRanges[j] == fRanges[i]
                     && rangeTok->fRanges[j+1] < fRanges[i+1])) {

            for (int count = 0; count < 2; count++) {
                result[k++] = rangeTok->fRanges[j++];
            }
        }
        else {

            for (int count = 0; count < 2; count++) {

                result[k++] = fRanges[i++];
            }
        }
    }

    fMemoryManager->deallocate(fRanges);//delete [] fRanges;
    fElemCount += rangeTok->fElemCount;
    fRanges = result;
    fMaxCount = newMaxCount;
}

void RangeToken::subtractRanges(RangeToken* const tok) {

    if (fRanges == 0 || tok->fRanges == 0)
        return;

    if (tok->getTokenType() == T_NRANGE) {

        intersectRanges(tok);
        return;
    }

    fCaseIToken = 0;
    sortRanges();
    compactRanges();
    tok->sortRanges();
    tok->compactRanges();

    unsigned int newMax = (fElemCount + tok->fElemCount >= fMaxCount)
                             ? fMaxCount + tok->fMaxCount : fMaxCount;
    XMLInt32* result = (XMLInt32*) fMemoryManager->allocate
    (
        newMax * sizeof(XMLInt32)
    );//new XMLInt32[newMax];
    unsigned int newElemCount = 0;
    unsigned int srcCount = 0;
    unsigned int subCount = 0;

    while (srcCount < fElemCount && subCount < tok->fElemCount) {

        XMLInt32 srcBegin = fRanges[srcCount];
        XMLInt32 srcEnd = fRanges[srcCount + 1];
        XMLInt32 subBegin = tok->fRanges[subCount];
        XMLInt32 subEnd = tok->fRanges[subCount + 1];

        if (srcEnd < subBegin) { // no overlap

            result[newElemCount++] = fRanges[srcCount++];
            result[newElemCount++] = fRanges[srcCount++];
        }
        else if (srcEnd >= subBegin && srcBegin <= subEnd) {

            if (subBegin <= srcBegin && srcEnd <= subEnd) {
                srcCount += 2;
            }
            else if (subBegin <= srcBegin) {

                fRanges[srcCount] = subEnd + 1;
                subCount += 2;
            }
            else if (srcEnd <= subEnd) {

                result[newElemCount++] = srcBegin;
                result[newElemCount++] = subBegin - 1;
                srcCount += 2;
            }
            else {

                result[newElemCount++] = srcBegin;
                result[newElemCount++] = subBegin - 1;
                fRanges[srcCount] = subEnd + 1;
                subCount += 2;
            }
        }
        else if (subEnd < srcBegin) {
            subCount += 2;
        }
        else {
            fMemoryManager->deallocate(result);//delete [] result;
            ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_SubtractRangesError, fMemoryManager);
        }
    } //end while

    while (srcCount < fElemCount) {

        result[newElemCount++] = fRanges[srcCount++];
        result[newElemCount++] = fRanges[srcCount++];
    }

    fMemoryManager->deallocate(fRanges);//delete [] fRanges;
    fRanges = result;
    fElemCount = newElemCount;
    fMaxCount = newMax;
}

/**
  * Ignore whether 'tok' is NRANGE or not.
  */
void RangeToken::intersectRanges(RangeToken* const tok) {

    if (fRanges == 0 || tok->fRanges == 0)
        return;

    fCaseIToken = 0;
    sortRanges();
    compactRanges();
    tok->sortRanges();
    tok->compactRanges();

    unsigned int newMax = (fElemCount + tok->fElemCount >= fMaxCount)
                             ? fMaxCount + tok->fMaxCount : fMaxCount;
    XMLInt32* result = (XMLInt32*) fMemoryManager->allocate
    (
        newMax * sizeof(XMLInt32)
    );//new XMLInt32[newMax];
    unsigned int newElemCount = 0;
    unsigned int srcCount = 0;
    unsigned int tokCount = 0;

    while (srcCount < fElemCount && tokCount < tok->fElemCount) {

        XMLInt32 srcBegin = fRanges[srcCount];
        XMLInt32 srcEnd = fRanges[srcCount + 1];
        XMLInt32 tokBegin = tok->fRanges[tokCount];
        XMLInt32 tokEnd = tok->fRanges[tokCount + 1];

        if (srcEnd < tokBegin) {
            srcCount += 2;
        }
        else if (srcEnd >= tokBegin && srcBegin <= tokEnd) {

            if (tokBegin <= srcBegin && srcEnd <= tokEnd) {

                result[newElemCount++] = srcBegin;
                result[newElemCount++] = srcEnd;
                srcCount += 2;
            }
            else if (tokBegin <= srcBegin) {

                result[newElemCount++] = srcBegin;
                result[newElemCount++] = tokEnd;
                tokCount += 2;

                if (tokCount < tok->fElemCount)
                    fRanges[srcCount] = tokEnd + 1;
                else
                    srcCount += 2;
            }
            else if (srcEnd <= tokEnd) {

                result[newElemCount++] = tokBegin;
                result[newElemCount++] = srcEnd;
                srcCount += 2;
            }
            else {

                result[newElemCount++] = tokBegin;
                result[newElemCount++] = tokEnd;
                tokCount += 2;

                if (tokCount < tok->fElemCount)
                    fRanges[srcCount] = tokEnd + 1;
                else
                    srcCount += 2;
            }
        }
        else if (tokEnd < srcBegin) {
            tokCount += 2;

            if (tokCount >= tok->fElemCount)
                srcCount += 2;
        }
        else {

            fMemoryManager->deallocate(result);//delete [] result;
            ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_IntersectRangesError, fMemoryManager);
        }
    } //end while

    fMemoryManager->deallocate(fRanges);//delete [] fRanges;
    fRanges = result;
    fElemCount = newElemCount;
    fMaxCount = newMax;
}

/**
  * for RANGE: Creates complement.
  * for NRANGE: Creates the same meaning RANGE.
  */
Token* RangeToken::complementRanges(RangeToken* const tok,
                                    TokenFactory* const tokFactory,
                                    MemoryManager* const manager) {

    if (tok->getTokenType() != T_RANGE && tok->getTokenType() != T_NRANGE)
        ThrowXMLwithMemMgr(IllegalArgumentException, XMLExcepts::Regex_ComplementRangesInvalidArg, manager);

    tok->sortRanges();
    tok->compactRanges();

    XMLInt32 lastElem = tok->fRanges[tok->fElemCount - 1];
    RangeToken* rangeTok = tokFactory->createRange();

    if (tok->fRanges[0] > 0) {
        rangeTok->addRange(0, tok->fRanges[0] - 1);
    }

    for (unsigned int i= 1; i< tok->fElemCount - 2; i += 2) {
        rangeTok->addRange(tok->fRanges[i] + 1, tok->fRanges[i+1] - 1);
    }

    if (lastElem != UTF16_MAX) {
        rangeTok->addRange(lastElem + 1, UTF16_MAX);
    }

    rangeTok->fCompacted = true;

    return rangeTok;
}


// ---------------------------------------------------------------------------
//  RangeToken: Match methods
// ---------------------------------------------------------------------------
bool RangeToken::match(const XMLInt32 ch) {

    createMap();

    bool ret;

    if (getTokenType() == T_RANGE) {

        if (ch < MAPSIZE)
            return ((fMap[ch/32] & (1<<(ch&0x1f))) != 0);

        ret = false;

        for (unsigned int i= fNonMapIndex; i< fElemCount; i +=2) {

            if (fRanges[i] <= ch && ch <= fRanges[i+1])
                return true;
        }
    }
    else {

        if (ch < MAPSIZE)
            return ((fMap[ch/32] & (1<<(ch&0x1f))) == 0);

        ret = true;

        for (unsigned int i= fNonMapIndex; i< fElemCount; i += 2) {

            if (fRanges[i] <= ch && ch <= fRanges[i+1])
                return false;
        }
    }

    return ret;
}

// ---------------------------------------------------------------------------
//  RangeToken: Private helpers methods
// ---------------------------------------------------------------------------
void RangeToken::expand(const unsigned int length) {

    unsigned int newMax = fElemCount + length;

    // Avoid too many reallocations by expanding by a percentage
    unsigned int minNewMax = (unsigned int)((double)fElemCount * 1.25);
    if (newMax < minNewMax)
        newMax = minNewMax;

    XMLInt32* newList = (XMLInt32*) fMemoryManager->allocate
    (
        newMax * sizeof(XMLInt32)
    );//new XMLInt32[newMax];
    for (unsigned int index = 0; index < fElemCount; index++)
        newList[index] = fRanges[index];

    fMemoryManager->deallocate(fRanges);//delete [] fRanges;
    fRanges = newList;
    fMaxCount = newMax;
}

void RangeToken::doCreateMap() {

    assert(!fMap);

    int asize = MAPSIZE/32;
    fMap = (int*) fMemoryManager->allocate(asize * sizeof(int));//new int[asize];
    fNonMapIndex = fElemCount;

    for (int i = 0; i < asize; i++) {
        fMap[i] = 0;
    }

    for (unsigned int j= 0; j < fElemCount; j += 2) {

        XMLInt32 begin = fRanges[j];
        XMLInt32 end = fRanges[j+1];

        if (begin < MAPSIZE) {

            for (int k = begin; k <= end && k < MAPSIZE; k++) {
                fMap[k/32] |= 1<<(k&0x1F);
            }
        }
        else {

            fNonMapIndex = j;
            break;
        }

        if (end >= MAPSIZE) {

            fNonMapIndex = j;
            break;
        }
    }
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file RangeToken.cpp
  */
