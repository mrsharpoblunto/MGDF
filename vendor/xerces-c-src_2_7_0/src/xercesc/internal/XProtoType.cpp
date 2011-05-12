/*
 * Copyright 2003-2004 The Apache Software Foundation.
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
 * $Id: XProtoType.cpp 191054 2005-06-17 02:56:35Z jberry $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/internal/XProtoType.hpp>
#include <xercesc/internal/XSerializeEngine.hpp>
#include <xercesc/util/XMLString.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/***
 *
 *  write the length of the class name
 *  write the class name
 *
 ***/
void XProtoType::store(XSerializeEngine& serEng) const
{
    int strLen = XMLString::stringLen((char*)fClassName);
	serEng << strLen;
	serEng.write(fClassName, strLen * sizeof(XMLByte));
}

/***
 *
 *  To verify that the content in the binary stream
 *  is the same as this class
 *
 ***/
void XProtoType::load(XSerializeEngine& serEng
                    , XMLByte* const    inName
                    , MemoryManager* const manager)
{
    if (!inName)
    {       
        ThrowXMLwithMemMgr(XSerializationException
               , XMLExcepts::XSer_ProtoType_Null_ClassName, manager);
    }

    // read and check class name length
    int      inNameLen = XMLString::stringLen((char*)inName); 
    int      classNameLen = 0;
    serEng >> classNameLen;

	if (classNameLen != inNameLen)
    {
        XMLCh value1[17];
        XMLCh value2[17];
        XMLString::binToText(inNameLen,    value1, 16, 10, manager);
        XMLString::binToText(classNameLen, value2, 16, 10, manager);

        ThrowXMLwithMemMgr2(XSerializationException
                , XMLExcepts::XSer_ProtoType_NameLen_Dif
                , value1
                , value2
                , manager);  
    }

    // read and check class name
	XMLByte  className[256];
    serEng.read(className, classNameLen*sizeof(XMLByte));
    className[classNameLen] = '\0';

    if ( !XMLString::equals((char*)className, (char*)inName))
    {
        //we don't have class name exceed this length in xerces
        XMLCh name1[256];
        XMLCh name2[256];
        XMLString::transcode((char*)inName,    name1, 255, manager);
        XMLString::transcode((char*)className, name2, 255, manager);

        ThrowXMLwithMemMgr2(XSerializationException
                , XMLExcepts::XSer_ProtoType_Name_Dif
                , name1
                , name2
                , manager);  
    }

    return;

}

XERCES_CPP_NAMESPACE_END

