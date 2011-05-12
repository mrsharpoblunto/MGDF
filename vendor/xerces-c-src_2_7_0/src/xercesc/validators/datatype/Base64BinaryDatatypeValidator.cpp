/*
 * Copyright 2001-2004 The Apache Software Foundation.
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
 * $Id: Base64BinaryDatatypeValidator.cpp 219911 2005-07-20 14:33:45Z cargilld $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/datatype/Base64BinaryDatatypeValidator.hpp>
#include <xercesc/validators/datatype/InvalidDatatypeFacetException.hpp>
#include <xercesc/validators/datatype/InvalidDatatypeValueException.hpp>
#include <xercesc/util/Base64.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Constructors and Destructor
// ---------------------------------------------------------------------------
Base64BinaryDatatypeValidator::Base64BinaryDatatypeValidator(MemoryManager* const manager)
:AbstractStringValidator(0, 0, 0, DatatypeValidator::Base64Binary, manager)
{}

Base64BinaryDatatypeValidator::~Base64BinaryDatatypeValidator()
{}

Base64BinaryDatatypeValidator::Base64BinaryDatatypeValidator(
                          DatatypeValidator*            const baseValidator
                        , RefHashTableOf<KVStringPair>* const facets
                        , RefArrayVectorOf<XMLCh>*      const enums
                        , const int                           finalSet
                        , MemoryManager* const                manager)
:AbstractStringValidator(baseValidator, facets, finalSet, DatatypeValidator::Base64Binary, manager)
{
    init(enums, manager);
}

DatatypeValidator* Base64BinaryDatatypeValidator::newInstance
(
      RefHashTableOf<KVStringPair>* const facets
    , RefArrayVectorOf<XMLCh>* const      enums
    , const int                           finalSet
    , MemoryManager* const                manager
)
{
    return (DatatypeValidator*) new (manager) Base64BinaryDatatypeValidator(this, facets, enums, finalSet, manager);
}

// ---------------------------------------------------------------------------
//  Utilities
// ---------------------------------------------------------------------------

void Base64BinaryDatatypeValidator::checkValueSpace(const XMLCh* const content
                                                    , MemoryManager* const manager)
{
    if (!content || !*content)
        return;
    if (getLength(content, manager) < 0)
    { 
        ThrowXMLwithMemMgr1(InvalidDatatypeValueException
                , XMLExcepts::VALUE_Not_Base64
                , content
                , manager);
    }
}

int Base64BinaryDatatypeValidator::getLength(const XMLCh* const content
                                         , MemoryManager* const manager) const
{
    if (!content || !*content)
        return 0;
    return Base64::getDataLength(content, manager, Base64::Conf_Schema);
}

void Base64BinaryDatatypeValidator::normalizeEnumeration(MemoryManager* const manager)
{

    int enumLength = getEnumeration()->size();
    for ( int i=0; i < enumLength; i++)
    {
        XMLString::removeWS(getEnumeration()->elementAt(i), manager);
    }

}

void Base64BinaryDatatypeValidator::normalizeContent(XMLCh* const content
                                                     , MemoryManager* const manager) const
{
    XMLString::removeWS(content, manager);     
}

/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(Base64BinaryDatatypeValidator)

void Base64BinaryDatatypeValidator::serialize(XSerializeEngine& serEng)
{
    AbstractStringValidator::serialize(serEng);
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file Base64BinaryDatatypeValidator.cpp
  */
