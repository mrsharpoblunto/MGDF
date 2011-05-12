/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)

   The functionality in this class was based on code and explanation found in
   Debugging Applications for Microsoft .NET and Microsoft Windows (2003) 
   by John Robbins, and the Inside Windows columns for MSDN Magazine by Matt
   Pietrek, February and March 2002.
   http://msdn.microsoft.com/msdnmag/issues/02/02/PE/default.aspx
   http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/default.aspx
   (Any errors in style or functionality are mine. -Maria)
 * -------------------------------------------------------------------------- */
///  @file ImportHooker.cpp

#include "ImportHooker.h"
#include "Logger.h"

#include <stdio.h>

// RVAs are represented as DWORDs.  This makes it easier to add them together
// to use as a regular pointer.
#define MakePtr(cast, ptr, addValue) (cast)((DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

namespace WinUnitLib
{
    /// This function takes an array of HOOKFUNCDESC
    /// structures, where the function name is a name of the function whose 
    /// implementation is to be replaced, and the function pointer is the 
    /// function to replace it.  It also takes an HMODULE (the library whose 
    /// imports are to be hooked/replaced) and the name of an import module 
    /// that the function imports are expected to be in.
    /// @remarks 
    /// The pProc member of the HOOKFUNCDESC structures can be NULL and they 
    /// will simply be skipped.
    /// @return true if successful; otherwise false.
    BOOL ImportHooker::HookImportedFunctionsByName(
        HMODULE hModule,            ///< The HMODULE for the library whose imports are to be hooked.
        LPCSTR szImportMod,         ///< The name of the DLL whose functions are imported.
        UINT uiCount,               ///< Number of HOOKFUNCDESC objects being passed in in paHookArray.
        const HOOKFUNCDESC* paHookArray,  ///< Array of HOOKFUNCDESC objects.
        PROC* paOrigFuncs,          ///< Pointers to the original functions in 
                                    ///< case we want to reset them or call them
        DWORD* pdwHooked,           ///< How many were hooked
        Logger& logger)            ///< Logger to write error messages to.
    {
        // Check parameters.
        //
        if (paOrigFuncs != NULL && 
            IsBadWritePtr(paOrigFuncs, sizeof(PROC)*uiCount))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": paOrigFuncs is invalid.");
            return FALSE;
        }
        if (pdwHooked != NULL &&
            IsBadWritePtr(pdwHooked, sizeof(UINT)))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": pdwHooked is invalid.");            
            return FALSE;
        }
        if (IsBadReadPtr(hModule, sizeof(IMAGE_DOS_HEADER)))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": hModule is invalid.");
            return FALSE;
        }
        if (IsBadStringPtrA(szImportMod, MAX_PATH))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": szImportMod is an invalid string.");
            return FALSE;
        }
        if (paHookArray == NULL)
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": paHookArray is NULL.");
            return FALSE;
        }
        if (IsBadReadPtr(paHookArray, sizeof(HOOKFUNCDESC) * uiCount))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": paHookArray is invalid.");
            return FALSE;
        }

        // Check each item in the hook array.
        for (UINT i = 0; i < uiCount; i++ )
        {
            if (paHookArray[i].szFunc == NULL || 
                ::IsBadStringPtrA(paHookArray[i].szFunc, MAX_PATH) ||
                *(paHookArray[i].szFunc) == '\0')
            {
                logger.OutputProgramErrorLine(__WFUNCTION__ L": paHookArray contains invalid function names.");
                return FALSE;
            }
            // If the function address isn't NULL, it is validated.
            if (paHookArray[i].pProc && IsBadCodePtr(paHookArray[i].pProc))
            {
                logger.OutputProgramErrorLine(__WFUNCTION__ L": paHookArray contains invalid function pointers.");
                return FALSE;
            }
        }

        // Initialize paOrigFuncs and pdwHooked
        if (paOrigFuncs != NULL)
        {
            // Set all the values in paOrigFuncs to NULL.
            memset(paOrigFuncs, NULL, sizeof(PROC) * uiCount ) ;
        }
        if (pdwHooked != NULL)
        {
            // Set the number of functions hooked to 0.
            *pdwHooked = 0;
        }

        // Get the import descriptor for the requested import module.
        PIMAGE_IMPORT_DESCRIPTOR pImportDesc = GetNamedImportDescriptor(hModule, szImportMod, logger);
        if(pImportDesc == NULL)
        {
            // The requested module wasn't imported. Don't return an error.
            return TRUE;
        }

        // Make sure the original first thunk and first thunk are not
        // NULL.  The orignal first thunk can be a null import
        // descriptor and cause this routine to crash.
        if ((pImportDesc->OriginalFirstThunk == NULL) ||
            (pImportDesc->FirstThunk == NULL))
        {
            // I return true here because it's analogous to the case in
            // which the requested module wasn't imported.
            return TRUE;
        }

        // Get the original thunk information for this DLL. I can't use
        // the thunk information stored in pImportDesc->FirstThunk
        // because the loader has already changed that array to fix up
        // all the imports. The original thunk gives me access to the
        // function names.
        PIMAGE_THUNK_DATA pOrigThunk =
                            MakePtr(PIMAGE_THUNK_DATA,
                                    hModule,
                                    pImportDesc->OriginalFirstThunk);

        // Get the array the pImportDesc->FirstThunk points to because
        // I'll do the actual hooking there.
        PIMAGE_THUNK_DATA pRealThunk = MakePtr(PIMAGE_THUNK_DATA,
                                               hModule,
                                               pImportDesc->FirstThunk);

        // Loop through and find the functions to hook.
        while (pOrigThunk->u1.Function != NULL)
        {
            // Look only at functions that are imported by name, not those
            // that are imported by ordinal value. (If the high bit is set, you 
            // know the union u1 is an AddressOfData as opposed to an Ordinal.) 
            if (IMAGE_ORDINAL_FLAG !=
                            (pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG))
            {
                // Look at the name of this imported function.
                PIMAGE_IMPORT_BY_NAME pByName ;

                pByName = MakePtr(PIMAGE_IMPORT_BY_NAME,
                                  hModule,
                                  pOrigThunk->u1.AddressOfData);

                // If the name starts with NULL, skip it.
                if (pByName->Name[0] == '\0')
                {
                    pOrigThunk++ ;
                    pRealThunk++ ;
                    continue ;
                }

                // Determines whether I hook the function
                bool bDoHook = FALSE;

                // See whether the imported function name is in the hook
                // array.
                UINT i;
                for(i = 0; i < uiCount; i++)
                {
                    if(_stricmp(paHookArray[i].szFunc, (char*)pByName->Name) == 0)
                    {
                        // If the function address is NULL, exit now;
                        // otherwise, go ahead and hook the function.
                        if (paHookArray[i].pProc != NULL)
                        {
                            bDoHook = true ;
                        }
                        break ;
                    }
                }

                if (bDoHook)
                {
                    // I found a function to hook. Now I need to change
                    // the memory protection to writable before I overwrite
                    // the function pointer. Note that I'm now writing into
                    // the real thunk area!

                    MEMORY_BASIC_INFORMATION mbi_thunk ;

                    VirtualQuery(pRealThunk,
                                 &mbi_thunk,
                                 sizeof(MEMORY_BASIC_INFORMATION));

                    if (!VirtualProtect(mbi_thunk.BaseAddress,
                                        mbi_thunk.RegionSize,
                                        PAGE_READWRITE,
                                        &mbi_thunk.Protect))
                    {
                        logger.OutputProgramErrorLine(__WFUNCTION__ L": VirtualProtect failed.");
                        return FALSE;
                    }

                    // Save the original address if requested.
                    if (paOrigFuncs != NULL)
                    {
                        paOrigFuncs[i] =
                            (PROC)((INT_PTR)pRealThunk->u1.Function) ;
                    }

                    // Hook the function.
                    DWORD_PTR* pTemp = (DWORD_PTR*)&pRealThunk->u1.Function ;
                    *pTemp = (DWORD_PTR)(paHookArray[i].pProc);

                    // Change the protection back to what it was before I
                    // overwrote the function pointer.
                    DWORD dummy;
                    if (!VirtualProtect(mbi_thunk.BaseAddress,
                                        mbi_thunk.RegionSize,
                                        mbi_thunk.Protect,
                                        &dummy))
                    {
                        logger.OutputProgramErrorLine(__WFUNCTION__ L": VirtualProtect failed.");
                        return FALSE;
                    }

                    if (pdwHooked != NULL)
                    {
                        // Increment the total number of functions hooked.
                        *pdwHooked += 1 ;
                    }
                }
            }
            // Increment both tables.
            pOrigThunk++ ;
            pRealThunk++ ;
        }

        return TRUE;
    }

    /// This is just a way to break out a large section in HookImportedFunctionsByName
    /// into a separate task for readability.  It finds the "import descriptor"
    /// for a module of a certain name, in order to then look for particular
    /// imports from that module.
    /// @return A pointer to the import descriptor, if any; otherwise NULL.
    PIMAGE_IMPORT_DESCRIPTOR ImportHooker::GetNamedImportDescriptor (
        HMODULE hModule,    ///< The HMODULE of the executable we're looking in.
        LPCSTR szImportMod, ///< The name of the module whose exports we're looking for.
        Logger& logger)    ///< Logger to write errors to.
    {
        // Always check parameters.
        if ((szImportMod == NULL) || (hModule == NULL))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": szImportMod cannot be NULL.");
            return NULL;
        }

        PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

        // Is this the MZ header?
        if (IsBadReadPtr(pDOSHeader, sizeof(IMAGE_DOS_HEADER)) ||
            (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": Invalid PE file.");
            return NULL;
        }

        // Get the PE header.
        PIMAGE_NT_HEADERS pNTHeader = MakePtr(PIMAGE_NT_HEADERS,
                                              pDOSHeader,
                                              pDOSHeader->e_lfanew);

        // Is this a real PE image?
        if (IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) ||
            (pNTHeader->Signature != IMAGE_NT_SIGNATURE))
        {
            logger.OutputProgramErrorLine(__WFUNCTION__ L": Invalid PE file.");
            return NULL;
        }

        // If there is no imports section, leave now.
        if (pNTHeader->OptionalHeader.
                             DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
                                                          VirtualAddress == 0)
        {
            return NULL;
        }

        // Get the pointer to the imports section.
        PIMAGE_IMPORT_DESCRIPTOR pImportDesc
            = MakePtr(PIMAGE_IMPORT_DESCRIPTOR,
                      pDOSHeader,
                      pNTHeader->OptionalHeader.
                             DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
                                                          VirtualAddress);

        // Loop through the import module descriptors looking for the
        // module whose name matches szImportMod.
        while ( NULL != pImportDesc->Name )
        {
            PSTR szCurrMod = MakePtr(PSTR,
                                     pDOSHeader,
                                     pImportDesc->Name);
            if (0 == ::_stricmp(szCurrMod, szImportMod))
            {
                // Found it.
                break ;
            }
            // Look at the next one.
            pImportDesc++ ;
        }

        // If the name is NULL, then the module is not imported.
        if (pImportDesc->Name == NULL)
        {
            return NULL;
        }

        return pImportDesc;
    }
}