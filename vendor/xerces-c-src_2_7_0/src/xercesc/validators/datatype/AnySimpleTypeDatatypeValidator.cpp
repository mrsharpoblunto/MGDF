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
 * $Id: AnySimpleTypeDatatypeValidator.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/datatype/AnySimpleTypeDatatypeValidator.hpp>
#include <xercesc/util/RuntimeException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  AnySimpleTypeDatatypeValidator: Constructors and Destructor
// ---------------------------------------------------------------------------
AnySimpleTypeDatatypeValidator::AnySimpleTypeDatatypeValidator(MemoryManager* const manager)
    : DatatypeValidator(0, 0, SchemaSymbols::XSD_RESTRICTION, DatatypeValidator::AnySimpleType, manager)
{
    setWhiteSpace(DatatypeValidator::PRESERVE);
    setFinite(true);
}

AnySimpleTypeDatatypeValidator::~AnySimpleTypeDatatypeValidator()
{

}

// ---------------------------------------------------------------------------
//  AnySimpleTypeDatatypeValidator: Instance methods
// ---------------------------------------------------------------------------
DatatypeValidator* AnySimpleTypeDatatypeValidator::newInstance
(
      RefHashTableOf<KVStringPair>* const facets
    , RefArrayVectorOf<XMLCh>* const      enums
    , const int
    , MemoryManager* const                manager
)
{
    // We own them, so we will delete them first
    delete facets;
    delete enums;

    ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::DV_InvalidOperation, manager);

    // to satisfy some compilers
    return 0;
}

const RefArrayVectorOf<XMLCh>* AnySimpleTypeDatatypeValidator::getEnumString() const
{
	return 0;
}

/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(AnySimpleTypeDatatypeValidator)

void AnySimpleTypeDatatypeValidator::serialize(XSerializeEngine& serEng)
{
    DatatypeValidator::serialize(serEng);
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file AnySimpleTypeDatatypeValidator.cpp
  */

