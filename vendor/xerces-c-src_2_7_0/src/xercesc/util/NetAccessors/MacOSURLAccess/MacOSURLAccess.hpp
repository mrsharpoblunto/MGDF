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
 * $Id: MacOSURLAccess.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


#if !defined(MACOSURLACCESS_HPP)
#define MACOSURLACCESS_HPP


#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLURL.hpp>
#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/XMLNetAccessor.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
// This class is the wrapper for the Mac OS URLAccess code
// which provides access to network resources.
// It's being used here to add the ability to
// use HTTP URL's as the system id's in the XML decl clauses.
//

class XMLUTIL_EXPORT MacOSURLAccess : public XMLNetAccessor
{
public :
    MacOSURLAccess();
    ~MacOSURLAccess();

    BinInputStream* makeNew(const XMLURL&  urlSource, const XMLNetHTTPInfo* httpInfo=0);
    const XMLCh* getId() const;

private :
    static const XMLCh sMyID[];

    MacOSURLAccess(const MacOSURLAccess&);
    MacOSURLAccess& operator=(const MacOSURLAccess&);

};

inline const XMLCh*
MacOSURLAccess::getId() const
{
    return sMyID;
}


XERCES_CPP_NAMESPACE_END


#endif // MACOSURLACCESS_HPP
