/*
 * Copyright 2003,2004 The Apache Software Foundation.
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
 * $Id: ValidationContextImpl.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/internal/ValidationContextImpl.hpp>
#include <xercesc/framework/XMLRefInfo.hpp>
#include <xercesc/validators/DTD/DTDEntityDecl.hpp>
#include <xercesc/validators/datatype/InvalidDatatypeValueException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Constructor and Destructor
// ---------------------------------------------------------------------------
ValidationContextImpl::~ValidationContextImpl()
{
    if (fIdRefList)
        delete fIdRefList;
}

ValidationContextImpl::ValidationContextImpl(MemoryManager* const manager)
:ValidationContext(manager)
,fIdRefList(0)
,fEntityDeclPool(0)
,fToCheckIdRefList(true)
,fValidatingMemberType(0)
{
    fIdRefList = new (fMemoryManager) RefHashTableOf<XMLRefInfo>(109, fMemoryManager);
}

/**
 * IdRefList
 *
 */
RefHashTableOf<XMLRefInfo>* ValidationContextImpl::getIdRefList() const
{
    return fIdRefList;
}

void ValidationContextImpl::setIdRefList(RefHashTableOf<XMLRefInfo>* const newIdRefList)
{
    if (fIdRefList)
        delete fIdRefList;

    fIdRefList = newIdRefList;
}

void ValidationContextImpl::clearIdRefList()
{
    if (fIdRefList)
        fIdRefList->removeAll();

}

void ValidationContextImpl::addId(const XMLCh * const content)
{
    if (!fIdRefList || !fToCheckIdRefList)
        return;

    XMLRefInfo* idEntry = fIdRefList->get(content);

    if (idEntry)
    {
        if (idEntry->getDeclared())
        {
            ThrowXMLwithMemMgr1(InvalidDatatypeValueException
                    , XMLExcepts::VALUE_ID_Not_Unique
                    , content
                    , fMemoryManager);
        }
    }
    else
    {
        idEntry = new (fMemoryManager) XMLRefInfo(content, false, false, fMemoryManager);
        fIdRefList->put((void*)idEntry->getRefName(), idEntry);
    }

    //
    //  Mark it declared
    //
    idEntry->setDeclared(true);

}

void ValidationContextImpl::addIdRef(const XMLCh * const content)
{
    if (!fIdRefList || !fToCheckIdRefList)
        return;

    XMLRefInfo* idEntry = fIdRefList->get(content);

    if (!idEntry)
    {
        idEntry = new (fMemoryManager) XMLRefInfo(content, false, false, fMemoryManager);
        fIdRefList->put((void*)idEntry->getRefName(), idEntry);
    }

    //
    //  Mark it used
    //
    idEntry->setUsed(true);

}

void ValidationContextImpl::toCheckIdRefList(bool toCheck)
{ 
    fToCheckIdRefList = toCheck;
}

/**
 * EntityDeclPool
 *
 */
const NameIdPool<DTDEntityDecl>* ValidationContextImpl::getEntityDeclPool() const
{
    return fEntityDeclPool;
}

const NameIdPool<DTDEntityDecl>* ValidationContextImpl::setEntityDeclPool(const NameIdPool<DTDEntityDecl>* const newEntityDeclPool)
{
    // we don't own it so we return the existing one for the owner to delete
    const NameIdPool<DTDEntityDecl>* tempPool = fEntityDeclPool;
    fEntityDeclPool = newEntityDeclPool;
    return tempPool;

}
           
void ValidationContextImpl::checkEntity(const XMLCh * const content) const
{

    if (fEntityDeclPool)
    {
        const DTDEntityDecl* decl = fEntityDeclPool->getByKey(content);

        if (!decl || !decl->isUnparsed())
        {
            ThrowXMLwithMemMgr1(InvalidDatatypeValueException
                    , XMLExcepts::VALUE_ENTITY_Invalid
                    , content
                    , fMemoryManager);
        }

    }
    else 
    {
        ThrowXMLwithMemMgr1
        (
            InvalidDatatypeValueException
            , XMLExcepts::VALUE_ENTITY_Invalid
            , content
            , fMemoryManager
        );
    }

}

XERCES_CPP_NAMESPACE_END

