/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: DTDAttDefList.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/DTD/DTDAttDefList.hpp>
#include <xercesc/internal/XTemplateSerializer.hpp>
#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  DTDAttDefList: Constructors and Destructor
// ---------------------------------------------------------------------------
DTDAttDefList::DTDAttDefList(RefHashTableOf<DTDAttDef>* const listToUse, MemoryManager* const manager)
: XMLAttDefList(manager)
,fEnum(0)
,fList(listToUse)
,fArray(0)
,fSize(0)
,fCount(0)
{
    fEnum = new (getMemoryManager()) RefHashTableOfEnumerator<DTDAttDef>(listToUse, false, manager);
    fArray = (DTDAttDef **)(manager->allocate( sizeof(DTDAttDef*) << 1));
    fSize = 2;
}

DTDAttDefList::~DTDAttDefList()
{
    delete fEnum;
    (getMemoryManager())->deallocate(fArray);
}


// ---------------------------------------------------------------------------
//  DTDAttDefList: Implementation of the virtual interface
// ---------------------------------------------------------------------------
bool DTDAttDefList::hasMoreElements() const
{
    return fEnum->hasMoreElements();
}


bool DTDAttDefList::isEmpty() const
{
    return fList->isEmpty();
}


XMLAttDef* DTDAttDefList::findAttDef(const  unsigned long
                                    , const XMLCh* const    attName)
{
    // We don't use the URI, so we just look up the name
    return fList->get(attName);
}


const XMLAttDef*
DTDAttDefList::findAttDef(  const   unsigned long
                            , const XMLCh* const    attName) const
{
    // We don't use the URI, so we just look up the name
    return fList->get(attName);
}


XMLAttDef* DTDAttDefList::findAttDef(   const   XMLCh* const
                                        , const XMLCh* const    attName)
{
    // We don't use the URI, so we just look up the name
    return fList->get(attName);
}


const XMLAttDef*
DTDAttDefList::findAttDef(  const   XMLCh* const
                            , const XMLCh* const    attName) const
{
    // We don't use the URI, so we just look up the name
    return fList->get(attName);
}


XMLAttDef& DTDAttDefList::nextElement()
{
    return fEnum->nextElement();
}


void DTDAttDefList::Reset()
{
    fEnum->Reset();
}

/**
 * return total number of attributes in this list
 */
unsigned int DTDAttDefList::getAttDefCount() const
{
    return fCount;
}

/**
 * return attribute at the index-th position in the list.
 */
XMLAttDef &DTDAttDefList::getAttDef(unsigned int index) 
{
    if(index >= fCount)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::AttrList_BadIndex, getMemoryManager());
    return *(fArray[index]);
}

/**
 * return attribute at the index-th position in the list.
 */
const XMLAttDef &DTDAttDefList::getAttDef(unsigned int index) const 
{
    if(index >= fCount)
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::AttrList_BadIndex, getMemoryManager());
    return *(fArray[index]);
}

/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(DTDAttDefList)

void DTDAttDefList::serialize(XSerializeEngine& serEng)
{

    XMLAttDefList::serialize(serEng);

    if (serEng.isStoring())
    {
        /***
         *
         * Serialize RefHashTableOf<DTDAttDef>           
         *
         ***/
        XTemplateSerializer::storeObject(fList, serEng);
        serEng << fCount;

        // do not serialize fEnum
    }
    else
    {
        /***
         *
         * Deserialize RefHashTableOf<DTDAttDef>           
         *
         ***/
        XTemplateSerializer::loadObject(&fList, 29, true, serEng);
        // assume empty so we can size fArray just right
        serEng >> fSize;
        if (!fEnum && fList)
        {
             fEnum = new (getMemoryManager()) RefHashTableOfEnumerator<DTDAttDef>(fList, false, getMemoryManager());
        }
        if(fSize) 
        {
            (getMemoryManager())->deallocate(fArray);
            fArray = (DTDAttDef **)((getMemoryManager())->allocate( sizeof(DTDAttDef*) * fSize));
            fCount = 0;
            while(fEnum->hasMoreElements())
            {
                fArray[fCount++] = &fEnum->nextElement();
            }
        }
    }

}

	
DTDAttDefList::DTDAttDefList(MemoryManager* const manager)
: XMLAttDefList(manager)
,fEnum(0)
,fList(0)
,fArray(0)
,fSize(0)
,fCount(0)
{
}

XERCES_CPP_NAMESPACE_END

