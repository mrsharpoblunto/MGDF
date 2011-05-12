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
 * $Id: URLAccessBinInputStream.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(URLACCESSBININPUTSTREAM_HPP)
#define URLACCESSBININPUTSTREAM_HPP


#include <xercesc/util/XMLURL.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/BinInputStream.hpp>

#if defined(__APPLE__)
    //	Framework includes from ProjectBuilder
	#include <Carbon/Carbon.h>
#else
    //	Classic includes otherwise
	#include <URLAccess.h>
#endif

XERCES_CPP_NAMESPACE_BEGIN

//
// This class implements the BinInputStream interface specified by the XML
// parser.
//

class XMLUTIL_EXPORT URLAccessBinInputStream : public BinInputStream
{
public :
    URLAccessBinInputStream(const XMLURL&  urlSource);
    ~URLAccessBinInputStream();

    unsigned int curPos() const;
    unsigned int readBytes
    (
                XMLByte* const  toFill
        , const unsigned int    maxToRead
    );


private :
    unsigned int        mBytesProcessed;
    URLReference		mURLReference;
    void*				mBuffer;			// Current buffer from URLAccess (or NULL)
    char*				mBufPos;			// Read position in buffer
    Size				mBufAvailable;		// Bytes available
};


inline unsigned int
URLAccessBinInputStream::curPos() const
{
    return mBytesProcessed;
}

XERCES_CPP_NAMESPACE_END

#endif // URLACCESSBININPUTSTREAM_HPP
