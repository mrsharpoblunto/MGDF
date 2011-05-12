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
 * $Id: OpFactory.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/Op.hpp>
#include <xercesc/util/regx/OpFactory.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  OpFactory: Constructors and Destructor
// ---------------------------------------------------------------------------
OpFactory::OpFactory(MemoryManager* const manager) :
    fOpVector(0)
    , fMemoryManager(manager)
{
    fOpVector = new (fMemoryManager) RefVectorOf<Op>(16, true, fMemoryManager);
}

OpFactory::~OpFactory() {

	delete fOpVector;
	fOpVector = 0;
}

// ---------------------------------------------------------------------------
//  OpFactory - Factory methods
// ---------------------------------------------------------------------------
Op* OpFactory::createDotOp() {

	Op* tmpOp = new (fMemoryManager) Op(Op::O_DOT, fMemoryManager);
	fOpVector->addElement(tmpOp);
	return tmpOp;
}

CharOp* OpFactory::createCharOp(XMLInt32 data) {

	CharOp* tmpOp = new (fMemoryManager) CharOp(Op::O_CHAR, data, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

CharOp* OpFactory::createAnchorOp(XMLInt32 data) {

	CharOp* tmpOp = new (fMemoryManager) CharOp(Op::O_ANCHOR, data, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

CharOp* OpFactory::createCaptureOp(int number, const Op* const next) {

	CharOp* tmpOp = new (fMemoryManager) CharOp(Op::O_CAPTURE, number, fMemoryManager);

	tmpOp->setNextOp(next);
	fOpVector->addElement(tmpOp);
	return tmpOp;
}

UnionOp* OpFactory::createUnionOp(int size) {

	UnionOp* tmpOp = new (fMemoryManager) UnionOp(Op::O_UNION, size, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ChildOp* OpFactory::createClosureOp(int id) {

	ModifierOp* tmpOp = new (fMemoryManager) ModifierOp(Op::O_CLOSURE, id, -1, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ChildOp* OpFactory::createNonGreedyClosureOp() {

	ChildOp* tmpOp = new (fMemoryManager) ChildOp(Op::O_NONGREEDYCLOSURE, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ChildOp* OpFactory::createQuestionOp(bool nonGreedy) {

	ChildOp* tmpOp = new (fMemoryManager)  ChildOp(nonGreedy ? Op::O_NONGREEDYQUESTION :
											 Op::O_QUESTION, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

RangeOp* OpFactory::createRangeOp(const Token* const token) {

	RangeOp* tmpOp = new (fMemoryManager)  RangeOp(Op::O_RANGE, token, fMemoryManager);
	
	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ChildOp* OpFactory::createLookOp(const short type, const Op* const next,
						         const Op* const branch) {

	ChildOp* tmpOp = new (fMemoryManager) ChildOp(type, fMemoryManager);

	tmpOp->setNextOp(next);
	tmpOp->setChild(branch);
	fOpVector->addElement(tmpOp);
	return tmpOp;
}

CharOp* OpFactory::createBackReferenceOp(int refNo) {

	CharOp* tmpOp = new (fMemoryManager) CharOp(Op::O_BACKREFERENCE, refNo, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

StringOp* OpFactory::createStringOp(const XMLCh* const literal) {

	StringOp* tmpOp = new (fMemoryManager) StringOp(Op::O_STRING, literal, fMemoryManager);

	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ChildOp* OpFactory::createIndependentOp(const Op* const next,
							            const Op* const branch) {

	ChildOp* tmpOp = new (fMemoryManager) ChildOp(Op::O_INDEPENDENT, fMemoryManager);

	tmpOp->setNextOp(next);
	tmpOp->setChild(branch);
	fOpVector->addElement(tmpOp);
	return tmpOp;
}

ModifierOp* OpFactory::createModifierOp(const Op* const next,
                                        const Op* const branch,
                                        const int add, const int mask) {

	ModifierOp* tmpOp = new (fMemoryManager) ModifierOp(Op::O_MODIFIER, add, mask, fMemoryManager);

	tmpOp->setNextOp(next);
	tmpOp->setChild(branch);
	return tmpOp;
}
	
ConditionOp* OpFactory::createConditionOp(const Op* const next, const int ref,
								          const Op* const conditionFlow,
								          const Op* const yesFlow,
								          const Op* const noFlow) {

	ConditionOp* tmpOp = new (fMemoryManager) ConditionOp(Op::O_CONDITION, ref, conditionFlow,
										 yesFlow, noFlow, fMemoryManager);

	tmpOp->setNextOp(next);
	return tmpOp;
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file OpFactory.cpp
  */
