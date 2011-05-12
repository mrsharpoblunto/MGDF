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
 * $Id: FieldValueMap.cpp 191701 2005-06-21 18:16:34Z cargilld $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/schema/identity/FieldValueMap.hpp>
#include <xercesc/util/Janitor.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

typedef JanitorMemFunCall<FieldValueMap>    CleanupType;

// ---------------------------------------------------------------------------
//  FieldValueMap: Constructors and Destructor
// ---------------------------------------------------------------------------
FieldValueMap::FieldValueMap(MemoryManager* const manager)
    : fFields(0)
    , fValidators(0)
    , fValues(0)
    , fMemoryManager(manager)
{
}

FieldValueMap::FieldValueMap(const FieldValueMap& other)
    : XMemory(other)
    , fFields(0)
    , fValidators(0)
    , fValues(0)
    , fMemoryManager(other.fMemoryManager)
{
    if (other.fFields) {
        CleanupType cleanup(this, &FieldValueMap::cleanUp);

        try {

                unsigned int valuesSize = other.fValues->size();

                fFields = new (fMemoryManager) ValueVectorOf<IC_Field*>(*(other.fFields));
                fValidators = new (fMemoryManager) ValueVectorOf<DatatypeValidator*>(*(other.fValidators));
                fValues = new (fMemoryManager) RefArrayVectorOf<XMLCh>(other.fFields->curCapacity(), true, fMemoryManager);

                for (unsigned int i=0; i<valuesSize; i++) {
                    fValues->addElement(XMLString::replicate(other.fValues->elementAt(i), fMemoryManager));
                }
        }
        catch(const OutOfMemoryException&)
        {
            cleanup.release();

            throw;
        }

        cleanup.release();
    }
}

FieldValueMap::~FieldValueMap()
{
    cleanUp();
}

// ---------------------------------------------------------------------------
//  FieldValueMap: Private helper methods.
// ---------------------------------------------------------------------------
void FieldValueMap::cleanUp()
{
    delete fFields;
    delete fValidators;
    delete fValues;
}

// ---------------------------------------------------------------------------
//  FieldValueMap: Helper methods
// ---------------------------------------------------------------------------
int FieldValueMap::indexOf(const IC_Field* const key) const {

    if (fFields) {

        unsigned int fieldSize = fFields->size();

        for (unsigned int i=0; i < fieldSize; i++) {
            if (fFields->elementAt(i) == key) {
                return i;
            }
        }
    }

    return -1;
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file FieldValueMap.cpp
  */

