#if (__OS400_TGTVRM__>=510)                               /* @01a */
    #pragma datamodel(P128)                               /* @01a */
#endif                                                    /* @01a */

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
 * $Id: OS400Defs.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


// ---------------------------------------------------------------------------
//  AIX runs in big endian mode
// ---------------------------------------------------------------------------
#define ENDIANMODE_BIG


// ---------------------------------------------------------------------------
//  And define our file handle abstraction
// ---------------------------------------------------------------------------
typedef void* FileHandle;
#if (__OS400_TGTVRM__>=510)                                /* @01a */  
     #pragma datamodel(pop)                                /* @01a */ 
#endif                                                     /* @01a */

