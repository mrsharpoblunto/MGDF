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
 * $Id: ENTITYDatatypeValidator.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/datatype/ENTITYDatatypeValidator.hpp>
#include <xercesc/validators/datatype/InvalidDatatypeValueException.hpp>
#include <xercesc/framework/XMLEntityDecl.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Constructors and Destructor
// ---------------------------------------------------------------------------
ENTITYDatatypeValidator::ENTITYDatatypeValidator(MemoryManager* const manager)
:StringDatatypeValidator(0, 0, 0, DatatypeValidator::ENTITY, manager)
{}

ENTITYDatatypeValidator::ENTITYDatatypeValidator(
                          DatatypeValidator*            const baseValidator
                        , RefHashTableOf<KVStringPair>* const facets
                        , RefArrayVectorOf<XMLCh>*      const enums
                        , const int                           finalSet
                        , MemoryManager* const                manager)
:StringDatatypeValidator(baseValidator, facets, finalSet, DatatypeValidator::ENTITY, manager)
{
    init(enums, manager);
}

ENTITYDatatypeValidator::~ENTITYDatatypeValidator()
{}

DatatypeValidator* ENTITYDatatypeValidator::newInstance
(
      RefHashTableOf<KVStringPair>* const facets
    , RefArrayVectorOf<XMLCh>* const      enums
    , const int                           finalSet
    , MemoryManager* const                manager
)
{
    return (DatatypeValidator*) new (manager) ENTITYDatatypeValidator(this, facets, enums, finalSet, manager);
}

// -----------------------------------------------------------------------
// Compare methods
// -----------------------------------------------------------------------
int ENTITYDatatypeValidator::compare(const XMLCh* const lValue
                                   , const XMLCh* const rValue
                                   ,       MemoryManager*     const)
{
    return ( XMLString::equals(lValue, rValue)? 0 : -1);
}

void ENTITYDatatypeValidator::validate(const XMLCh*             const content
                                     ,       ValidationContext* const context
                                     ,       MemoryManager*     const manager)
{
    // use StringDatatypeValidator (which in turn, invoke
    // the baseValidator) to validate content against
    // facets if any.
    //
    StringDatatypeValidator::validate(content, context, manager);

    //
    // parse the entity iff an EntityDeclPool is provided
    //
    if (context)
    {
        context->checkEntity(content);
    }

}

void ENTITYDatatypeValidator::checkValueSpace(const XMLCh* const content
                                              , MemoryManager* const manager)
{
    //
    // 3.3.11 check must: "NCName"
    //
    if ( !XMLString::isValidNCName(content))
    {
        ThrowXMLwithMemMgr1(InvalidDatatypeValueException
                , XMLExcepts::VALUE_Invalid_NCName
                , content
                , manager);
    }

}

/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(ENTITYDatatypeValidator)

void ENTITYDatatypeValidator::serialize(XSerializeEngine& serEng)
{
    StringDatatypeValidator::serialize(serEng);
}

XERCES_CPP_NAMESPACE_END

 /**
  * End of file ENTITYDatatypeValidator.cpp
  */
