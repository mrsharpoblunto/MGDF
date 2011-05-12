/*
 * Copyright 2004,2004 The Apache Software Foundation.
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
 * $Id: XMLUTF8Transcoder390.cpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/TranscodingException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/Transcoders/Uniconv390/XMLUTF8Transcoder390.hpp>
#include <xercesc/util/UTFDataFormatException.hpp>
#include <string.h>

XERCES_CPP_NAMESPACE_BEGIN
extern "OS" void TROTASC(const XMLByte * input,
                         XMLCh * output,
                         unsigned int * count,
                         XMLCh *table,
                         int STOP,
                         int * FLAG
                        );

//Add a long double in front of the table, the compiler will set the 
//table starting address on a double word boundary
struct temp{
   long double pad;
   XMLCh gFromTable[256];
};


static struct temp padding_temp={
 0,
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007
  , 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F
  , 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017
  , 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F
  , 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027
  , 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F
  , 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037
  , 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F
  , 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047
  , 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F
  , 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057
  , 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F
  , 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067
  , 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F
  , 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077
  , 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
  , 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};


// ---------------------------------------------------------------------------
//  Local static data
//
//  gUTFBytes
//      A list of counts of trailing bytes for each initial byte in the input.
//
//  gUTFByteIndicator
//      For a UTF8 sequence of n bytes, n>=2, the first byte of the
//      sequence must contain n 1's followed by precisely 1 0 with the
//      rest of the byte containing arbitrary bits.  This array stores
//      the required bit pattern for validity checking.
//  gUTFByteIndicatorTest
//      When bitwise and'd with the observed value, if the observed
//      value is correct then a result matching gUTFByteIndicator will
//      be produced.
//
//  gUTFOffsets
//      A list of values to offset each result char type, according to how
//      many source bytes when into making it.
//
//  gFirstByteMark
//      A list of values to mask onto the first byte of an encoded sequence,
//      indexed by the number of bytes used to create the sequence.
// ---------------------------------------------------------------------------
static const XMLByte gUTFBytes[256] =
{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ,   0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    ,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    ,   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    ,   3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

static const XMLByte gUTFByteIndicator[6] =
{
    0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};
static const XMLByte gUTFByteIndicatorTest[6] =
{
    0x80, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE
};

static const XMLUInt32 gUTFOffsets[6] =
{
    0, 0x3080, 0xE2080, 0x3C82080, 0xFA082080, 0x82082080
};

static const XMLByte gFirstByteMark[7] =
{
    0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};



// ---------------------------------------------------------------------------
//  XMLUTF8Transcoder390: Constructors and Destructor
// ---------------------------------------------------------------------------
XMLUTF8Transcoder390::XMLUTF8Transcoder390(const  XMLCh* const    encodingName
                                    , const unsigned int    blockSize
                                    , MemoryManager* const manager) :

    XMLTranscoder(encodingName, blockSize, manager)
{
}

XMLUTF8Transcoder390::~XMLUTF8Transcoder390()
{
}


// ---------------------------------------------------------------------------
//  XMLUTF8Transcoder390: Implementation of the transcoder API
// ---------------------------------------------------------------------------
unsigned int
XMLUTF8Transcoder390::transcodeFrom(const  XMLByte* const          srcData
                                , const unsigned int            srcCount
                                ,       XMLCh* const            toFill
                                , const unsigned int            maxChars
                                ,       unsigned int&           bytesEaten
                                ,       unsigned char* const    charSizes)
{
    // Watch for pathological scenario. Shouldn't happen, but...
    if (!srcCount || !maxChars)
        return 0;

    // If debugging, make sure that the block size is legal
    #if defined(XERCES_DEBUG)
    checkBlockSize(maxChars);
    #endif

    //
    //  Get pointers to our start and end points of the input and output
    //  buffers.
    //
    const XMLByte*  srcPtr = srcData;
    const XMLByte*  srcEnd = srcPtr + srcCount;
    XMLCh*          outPtr = toFill;
    XMLCh*          outEnd = outPtr + maxChars;
    unsigned char*  sizePtr = charSizes;
    unsigned int    countDone = (srcCount<maxChars)?srcCount:maxChars;
    // if this flag is set, a none ASCII character is encountered
    int             flag = 0;


    // Special-case ASCII, which is a leading byte value of <= 127
                 
    TROTASC(srcPtr, outPtr, &countDone, padding_temp.gFromTable, 0xFFFF,&flag);
    srcPtr+=countDone;
    outPtr+=countDone;
    memset(sizePtr, 1, countDone);
    sizePtr+=countDone;
    
    

    //
    //  We now loop until we either run out of input data, or room to store
    //  output chars.
    //
    while ((srcPtr < srcEnd) && (outPtr < outEnd))
    {
        
        if (*srcPtr <= 127)
        {
            // Handle ASCII in groups instead of single character at a time.
            const XMLByte* srcPtr_save = srcPtr;
            do
            {
                *outPtr++ = XMLCh(*srcPtr++);
            } while (*srcPtr <= 127    &&
                      srcPtr != srcEnd &&
                      outPtr != outEnd );
            memset(sizePtr,1,srcPtr - srcPtr_save);
            sizePtr += srcPtr - srcPtr_save;
            if (srcPtr == srcEnd || outPtr == outEnd)
                break;
        }
        
        // See how many trailing src bytes this sequence is going to require
        const unsigned int trailingBytes = gUTFBytes[*srcPtr];

        //
        //  If there are not enough source bytes to do this one, then we
        //  are done. Note that we done >= here because we are implicitly
        //  counting the 1 byte we get no matter what.
        //
        //  If we break out here, then there is nothing to undo since we
        //  haven't updated any pointers yet.
        //
        if (srcPtr + trailingBytes >= srcEnd)
            break;

        // Looks ok, so lets build up the value
        // or at least let's try to do so--remembering that
        // we cannot assume the encoding to be valid:

        // first, test first byte
        if((gUTFByteIndicatorTest[trailingBytes] & *srcPtr) != gUTFByteIndicator[trailingBytes]) {
            char pos[2] = {(char)0x31, 0}; 
            char len[2] = {(char)(trailingBytes+0x31), 0};
            char byte[2] = {*srcPtr,0};
            ThrowXMLwithMemMgr3(UTFDataFormatException, XMLExcepts::UTF8_FormatError, pos, byte, len, getMemoryManager());
        }

        /***
         * http://www.unicode.org/reports/tr27/
         *
         * Table 3.1B. lists all of the byte sequences that are legal in UTF-8. 
         * A range of byte values such as A0..BF indicates that any byte from A0 to BF (inclusive) 
         * is legal in that position. 
         * Any byte value outside of the ranges listed is illegal. 
         * For example, 
         * the byte sequence <C0 AF> is illegal  since C0 is not legal in the 1st Byte column. 
         * The byte sequence <E0 9F 80> is illegal since in the row 
         *    where E0 is legal as a first byte, 
         *    9F is not legal as a second byte. 
         * The byte sequence <F4 80 83 92> is legal, since every byte in that sequence matches 
         * a byte range in a row of the table (the last row). 
         *
         *
         * Table 3.1B. Legal UTF-8 Byte Sequences  
         * Code Points              1st Byte    2nd Byte    3rd Byte    4th Byte 
         * =========================================================================
         * U+0000..U+007F            00..7F       
         * -------------------------------------------------------------------------
         * U+0080..U+07FF            C2..DF      80..BF      
         *
         * -------------------------------------------------------------------------
         * U+0800..U+0FFF            E0          A0..BF     80..BF   
         *                                       -- 
         *                          
         * U+1000..U+FFFF            E1..EF      80..BF     80..BF    
         *
         * --------------------------------------------------------------------------
         * U+10000..U+3FFFF          F0          90..BF     80..BF       80..BF 
         *                                       --
         * U+40000..U+FFFFF          F1..F3      80..BF     80..BF       80..BF 
         * U+100000..U+10FFFF        F4          80..8F     80..BF       80..BF 
         *                                           --
         * ==========================================================================
         *
         *  Cases where a trailing byte range is not 80..BF are underlined in the table to 
         *  draw attention to them. These occur only in the second byte of a sequence.
         *
         ***/

        XMLUInt32 tmpVal = 0;

        switch(trailingBytes)
        {
            case 1 :
                // UTF-8:   [110y yyyy] [10xx xxxx]
                // Unicode: [0000 0yyy] [yyxx xxxx]
                //
                // 0xC0, 0xC1 has been filtered out             
                checkTrailingBytes(*(srcPtr+1), 1, 1);

                tmpVal = *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;

                break;
            case 2 :
                // UTF-8:   [1110 zzzz] [10yy yyyy] [10xx xxxx]
                // Unicode: [zzzz yyyy] [yyxx xxxx]
                //
                if (( *srcPtr == 0xE0) && ( *(srcPtr+1) < 0xA0)) 
                {
                    char byte0[2] = {*srcPtr    ,0};
                    char byte1[2] = {*(srcPtr+1),0};

                    ThrowXMLwithMemMgr2(UTFDataFormatException
                                      , XMLExcepts::UTF8_Invalid_3BytesSeq
                                      , byte0
                                      , byte1
                                      , getMemoryManager());
                }

                checkTrailingBytes(*(srcPtr+1), 2, 1);
                checkTrailingBytes(*(srcPtr+2), 2, 2);

                //
                // D36 (a) UTF-8 is the Unicode Transformation Format that serializes 
                //         a Unicode code point as a sequence of one to four bytes, 
                //         as specified in Table 3.1, UTF-8 Bit Distribution.
                //     (b) An illegal UTF-8 code unit sequence is any byte sequence that 
                //         does not match the patterns listed in Table 3.1B, Legal UTF-8 
                //         Byte Sequences.
                //     (c) An irregular UTF-8 code unit sequence is a six-byte sequence 
                //         where the first three bytes correspond to a high surrogate, 
                //         and the next three bytes correspond to a low surrogate. 
                //         As a consequence of C12, these irregular UTF-8 sequences shall 
                //         not be generated by a conformant process. 
                //
                //irregular three bytes sequence
                // that is zzzzyy matches leading surrogate tag 110110 or 
                //                       trailing surrogate tag 110111
                // *srcPtr=1110 1101 
                // *(srcPtr+1)=1010 yyyy or 
                // *(srcPtr+1)=1011 yyyy
                //
                // 0xED 1110 1101
                // 0xA0 1010 0000

                if ((*srcPtr == 0xED) && (*(srcPtr+1) >= 0xA0))
                {
                    char byte0[2] = {*srcPtr,    0};
                    char byte1[2] = {*(srcPtr+1),0};

                     ThrowXMLwithMemMgr2(UTFDataFormatException
                              , XMLExcepts::UTF8_Irregular_3BytesSeq
                              , byte0
                              , byte1
                              , getMemoryManager());
                }

                tmpVal = *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;

                break;
            case 3 : 
                // UTF-8:   [1111 0uuu] [10uu zzzz] [10yy yyyy] [10xx xxxx]*
                // Unicode: [1101 10ww] [wwzz zzyy] (high surrogate)
                //          [1101 11yy] [yyxx xxxx] (low surrogate)
                //          * uuuuu = wwww + 1
                //
                if (((*srcPtr == 0xF0) && (*(srcPtr+1) < 0x90)) ||
                    ((*srcPtr == 0xF4) && (*(srcPtr+1) > 0x8F))  )
                {
                    char byte0[2] = {*srcPtr    ,0};
                    char byte1[2] = {*(srcPtr+1),0};

                    ThrowXMLwithMemMgr2(UTFDataFormatException
                                      , XMLExcepts::UTF8_Invalid_4BytesSeq
                                      , byte0
                                      , byte1
                                      , getMemoryManager());
                }

                checkTrailingBytes(*(srcPtr+1), 3, 1);
                checkTrailingBytes(*(srcPtr+2), 3, 2);
                checkTrailingBytes(*(srcPtr+3), 3, 3);
                
                tmpVal = *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;
                tmpVal <<= 6;
                tmpVal += *srcPtr++;

                break;
            default: // trailingBytes > 3

                /***
                 * The definition of UTF-8 in Annex D of ISO/IEC 10646-1:2000 also allows 
                 * for the use of five- and six-byte sequences to encode characters that 
                 * are outside the range of the Unicode character set; those five- and 
                 * six-byte sequences are illegal for the use of UTF-8 as a transformation 
                 * of Unicode characters. ISO/IEC 10646 does not allow mapping of unpaired 
                 * surrogates, nor U+FFFE and U+FFFF (but it does allow other noncharacters).
                 ***/
                char len[2]  = {(char)(trailingBytes+0x31), 0};
                char byte[2] = {*srcPtr,0};

                ThrowXMLwithMemMgr2(UTFDataFormatException
                                  , XMLExcepts::UTF8_Exceede_BytesLimit
                                  , byte
                                  , len
                                  , getMemoryManager());

                break;
        }


        // since trailingBytes comes from an array, this logic is redundant
        //  default :
        //      ThrowXML(TranscodingException, XMLExcepts::Trans_BadSrcSeq);
        //}
        tmpVal -= gUTFOffsets[trailingBytes];

        //
        //  If it will fit into a single char, then put it in. Otherwise
        //  encode it as a surrogate pair. If its not valid, use the
        //  replacement char.
        //
        if (!(tmpVal & 0xFFFF0000))
        {
            *sizePtr++ = trailingBytes + 1;
            *outPtr++ = XMLCh(tmpVal);
        }
         else if (tmpVal > 0x10FFFF)
        {
            //
            //  If we've gotten more than 32 chars so far, then just break
            //  out for now and lets process those. When we come back in
            //  here again, we'll get no chars and throw an exception. This
            //  way, the error will have a line and col number closer to
            //  the real problem area.
            //
            if ((outPtr - toFill) > 32)
                break;

            ThrowXMLwithMemMgr(TranscodingException, XMLExcepts::Trans_BadSrcSeq, getMemoryManager());
        }
         else
        {
            //
            //  If we have enough room to store the leading and trailing
            //  chars, then lets do it. Else, pretend this one never
            //  happened, and leave it for the next time. Since we don't
            //  update the bytes read until the bottom of the loop, by
            //  breaking out here its like it never happened.
            //
            if (outPtr + 1 >= outEnd)
                break;

            // Store the leading surrogate char
            tmpVal -= 0x10000;
            *sizePtr++ = trailingBytes + 1;
            *outPtr++ = XMLCh((tmpVal >> 10) + 0xD800);

            //
            //  And then the trailing char. This one accounts for no
            //  bytes eaten from the source, so set the char size for this
            //  one to be zero.
            //
            *sizePtr++ = 0;
            *outPtr++ = XMLCh((tmpVal & 0x3FF) + 0xDC00);
        }
    }

    // Update the bytes eaten
    bytesEaten = srcPtr - srcData;

    // Return the characters read
    return outPtr - toFill;
}


unsigned int
XMLUTF8Transcoder390::transcodeTo( const   XMLCh* const    srcData
                                , const unsigned int    srcCount
                                ,       XMLByte* const  toFill
                                , const unsigned int    maxBytes
                                ,       unsigned int&   charsEaten
                                , const UnRepOpts       options)
{
    // Watch for pathological scenario. Shouldn't happen, but...
    if (!srcCount || !maxBytes)
        return 0;

    //
    //  Get pointers to our start and end points of the input and output
    //  buffers.
    //
    const XMLCh*    srcPtr = srcData;
    const XMLCh*    srcEnd = srcPtr + srcCount;
    XMLByte*        outPtr = toFill;
    XMLByte*        outEnd = toFill + maxBytes;

    while (srcPtr < srcEnd)
    {
        //
        //  Tentatively get the next char out. We have to get it into a
        //  32 bit value, because it could be a surrogate pair.
        //
        XMLUInt32 curVal = *srcPtr;

        //
        //  If its a leading surrogate, then lets see if we have the trailing
        //  available. If not, then give up now and leave it for next time.
        //
        unsigned int srcUsed = 1;
        if ((curVal >= 0xD800) && (curVal <= 0xDBFF))
        {
            if (srcPtr + 1 >= srcEnd)
                break;

            // Create the composite surrogate pair
            curVal = ((curVal - 0xD800) << 10)
                    + ((*(srcPtr + 1) - 0xDC00) + 0x10000);

            // And indicate that we ate another one
            srcUsed++;
        }

        // Figure out how many bytes we need
        unsigned int encodedBytes;
        if (curVal < 0x80)
            encodedBytes = 1;
        else if (curVal < 0x800)
            encodedBytes = 2;
        else if (curVal < 0x10000)
            encodedBytes = 3;
        else if (curVal < 0x110000)
            encodedBytes = 4;
        else
        {
            // If the options say to throw, then throw
            if (options == UnRep_Throw)
            {
                XMLCh tmpBuf[17];
                XMLString::binToText(curVal, tmpBuf, 16, 16, getMemoryManager());
                ThrowXMLwithMemMgr2
                (
                    TranscodingException
                    , XMLExcepts::Trans_Unrepresentable
                    , tmpBuf
                    , getEncodingName()
                    , getMemoryManager()
                );
            }

            // Else, use the replacement character
            *outPtr++ = chSpace;
            srcPtr += srcUsed;
            continue;
        }

        //
        //  If we cannot fully get this char into the output buffer,
        //  then leave it for the next time.
        //
        if (outPtr + encodedBytes > outEnd)
            break;

        // We can do it, so update the source index
        srcPtr += srcUsed;

        //
        //  And spit out the bytes. We spit them out in reverse order
        //  here, so bump up the output pointer and work down as we go.
        //
        outPtr += encodedBytes;
        switch(encodedBytes)
        {
            case 6 : *--outPtr = XMLByte((curVal | 0x80UL) & 0xBFUL);
                     curVal >>= 6;
            case 5 : *--outPtr = XMLByte((curVal | 0x80UL) & 0xBFUL);
                     curVal >>= 6;
            case 4 : *--outPtr = XMLByte((curVal | 0x80UL) & 0xBFUL);
                     curVal >>= 6;
            case 3 : *--outPtr = XMLByte((curVal | 0x80UL) & 0xBFUL);
                     curVal >>= 6;
            case 2 : *--outPtr = XMLByte((curVal | 0x80UL) & 0xBFUL);
                     curVal >>= 6;
            case 1 : *--outPtr = XMLByte
                     (
                        curVal | gFirstByteMark[encodedBytes]
                     );
        }

        // Add the encoded bytes back in again to indicate we've eaten them
        outPtr += encodedBytes;
    }

    // Fill in the chars we ate
    charsEaten = (srcPtr - srcData);

    // And return the bytes we filled in
    return (outPtr - toFill);
}


bool XMLUTF8Transcoder390::canTranscodeTo(const unsigned int toCheck) const
{
    // We can represent anything in the Unicode (with surrogates) range
    return (toCheck <= 0x10FFFF);
}

XERCES_CPP_NAMESPACE_END

