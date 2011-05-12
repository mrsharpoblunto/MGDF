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
 * $Id: MacOSURLAccessCF.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/NetAccessors/MacOSURLAccessCF/MacOSURLAccessCF.hpp>
#include <xercesc/util/NetAccessors/MacOSURLAccessCF/URLAccessCFBinInputStream.hpp>

XERCES_CPP_NAMESPACE_BEGIN

const XMLCh
MacOSURLAccessCF::sMyID[] =
{
    chLatin_M, chLatin_a, chLatin_c, chLatin_O, chLatin_S, chLatin_U,
    chLatin_R, chLatin_L, chLatin_A, chLatin_c, chLatin_c, chLatin_e,
    chLatin_s, chLatin_s, chLatin_C, chLatin_F,
    chNull
};


MacOSURLAccessCF::MacOSURLAccessCF()
{
}


MacOSURLAccessCF::~MacOSURLAccessCF()
{
}


BinInputStream*
MacOSURLAccessCF::makeNew(const XMLURL&  urlSource, const XMLNetHTTPInfo* httpInfo/*=0*/)
{
	if(httpInfo!=0 && httpInfo->fHTTPMethod!=XMLNetHTTPInfo::GET)
		ThrowXML(NetAccessorException, XMLExcepts::NetAcc_UnsupportedMethod);
	BinInputStream* result = new (urlSource.getMemoryManager()) URLAccessCFBinInputStream(urlSource);
	return result;
}

XERCES_CPP_NAMESPACE_END

