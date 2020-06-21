

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for MGDF.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __MGDF_h__
#define __MGDF_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMGDFDebug_FWD_DEFINED__
#define __IMGDFDebug_FWD_DEFINED__
typedef interface IMGDFDebug IMGDFDebug;

#endif 	/* __IMGDFDebug_FWD_DEFINED__ */


#ifndef __IMGDFPreferenceSet_FWD_DEFINED__
#define __IMGDFPreferenceSet_FWD_DEFINED__
typedef interface IMGDFPreferenceSet IMGDFPreferenceSet;

#endif 	/* __IMGDFPreferenceSet_FWD_DEFINED__ */


#ifndef __IMGDFGame_FWD_DEFINED__
#define __IMGDFGame_FWD_DEFINED__
typedef interface IMGDFGame IMGDFGame;

#endif 	/* __IMGDFGame_FWD_DEFINED__ */


#ifndef __IMGDFGamepad_FWD_DEFINED__
#define __IMGDFGamepad_FWD_DEFINED__
typedef interface IMGDFGamepad IMGDFGamepad;

#endif 	/* __IMGDFGamepad_FWD_DEFINED__ */


#ifndef __IMGDFInputManager_FWD_DEFINED__
#define __IMGDFInputManager_FWD_DEFINED__
typedef interface IMGDFInputManager IMGDFInputManager;

#endif 	/* __IMGDFInputManager_FWD_DEFINED__ */


#ifndef __IMGDFLogger_FWD_DEFINED__
#define __IMGDFLogger_FWD_DEFINED__
typedef interface IMGDFLogger IMGDFLogger;

#endif 	/* __IMGDFLogger_FWD_DEFINED__ */


#ifndef __IMGDFPerformanceCounter_FWD_DEFINED__
#define __IMGDFPerformanceCounter_FWD_DEFINED__
typedef interface IMGDFPerformanceCounter IMGDFPerformanceCounter;

#endif 	/* __IMGDFPerformanceCounter_FWD_DEFINED__ */


#ifndef __IMGDFTimer_FWD_DEFINED__
#define __IMGDFTimer_FWD_DEFINED__
typedef interface IMGDFTimer IMGDFTimer;

#endif 	/* __IMGDFTimer_FWD_DEFINED__ */


#ifndef __IMGDFRenderSettingsManager_FWD_DEFINED__
#define __IMGDFRenderSettingsManager_FWD_DEFINED__
typedef interface IMGDFRenderSettingsManager IMGDFRenderSettingsManager;

#endif 	/* __IMGDFRenderSettingsManager_FWD_DEFINED__ */


#ifndef __IMGDFFileReader_FWD_DEFINED__
#define __IMGDFFileReader_FWD_DEFINED__
typedef interface IMGDFFileReader IMGDFFileReader;

#endif 	/* __IMGDFFileReader_FWD_DEFINED__ */


#ifndef __IMGDFFile_FWD_DEFINED__
#define __IMGDFFile_FWD_DEFINED__
typedef interface IMGDFFile IMGDFFile;

#endif 	/* __IMGDFFile_FWD_DEFINED__ */


#ifndef __IMGDFArchiveHandler_FWD_DEFINED__
#define __IMGDFArchiveHandler_FWD_DEFINED__
typedef interface IMGDFArchiveHandler IMGDFArchiveHandler;

#endif 	/* __IMGDFArchiveHandler_FWD_DEFINED__ */


#ifndef __IMGDFVirtualFileSystem_FWD_DEFINED__
#define __IMGDFVirtualFileSystem_FWD_DEFINED__
typedef interface IMGDFVirtualFileSystem IMGDFVirtualFileSystem;

#endif 	/* __IMGDFVirtualFileSystem_FWD_DEFINED__ */


#ifndef __IMGDFSound_FWD_DEFINED__
#define __IMGDFSound_FWD_DEFINED__
typedef interface IMGDFSound IMGDFSound;

#endif 	/* __IMGDFSound_FWD_DEFINED__ */


#ifndef __IMGDFSoundStream_FWD_DEFINED__
#define __IMGDFSoundStream_FWD_DEFINED__
typedef interface IMGDFSoundStream IMGDFSoundStream;

#endif 	/* __IMGDFSoundStream_FWD_DEFINED__ */


#ifndef __IMGDFSoundManager_FWD_DEFINED__
#define __IMGDFSoundManager_FWD_DEFINED__
typedef interface IMGDFSoundManager IMGDFSoundManager;

#endif 	/* __IMGDFSoundManager_FWD_DEFINED__ */


#ifndef __IMGDFStatisticsManager_FWD_DEFINED__
#define __IMGDFStatisticsManager_FWD_DEFINED__
typedef interface IMGDFStatisticsManager IMGDFStatisticsManager;

#endif 	/* __IMGDFStatisticsManager_FWD_DEFINED__ */


#ifndef __IMGDFCommonHost_FWD_DEFINED__
#define __IMGDFCommonHost_FWD_DEFINED__
typedef interface IMGDFCommonHost IMGDFCommonHost;

#endif 	/* __IMGDFCommonHost_FWD_DEFINED__ */


#ifndef __IMGDFRenderHost_FWD_DEFINED__
#define __IMGDFRenderHost_FWD_DEFINED__
typedef interface IMGDFRenderHost IMGDFRenderHost;

#endif 	/* __IMGDFRenderHost_FWD_DEFINED__ */


#ifndef __IMGDFPendingSave_FWD_DEFINED__
#define __IMGDFPendingSave_FWD_DEFINED__
typedef interface IMGDFPendingSave IMGDFPendingSave;

#endif 	/* __IMGDFPendingSave_FWD_DEFINED__ */


#ifndef __IMGDFGameState_FWD_DEFINED__
#define __IMGDFGameState_FWD_DEFINED__
typedef interface IMGDFGameState IMGDFGameState;

#endif 	/* __IMGDFGameState_FWD_DEFINED__ */


#ifndef __IMGDFSaveManager_FWD_DEFINED__
#define __IMGDFSaveManager_FWD_DEFINED__
typedef interface IMGDFSaveManager IMGDFSaveManager;

#endif 	/* __IMGDFSaveManager_FWD_DEFINED__ */


#ifndef __IMGDFSimHost_FWD_DEFINED__
#define __IMGDFSimHost_FWD_DEFINED__
typedef interface IMGDFSimHost IMGDFSimHost;

#endif 	/* __IMGDFSimHost_FWD_DEFINED__ */


#ifndef __IMGDFModule_FWD_DEFINED__
#define __IMGDFModule_FWD_DEFINED__
typedef interface IMGDFModule IMGDFModule;

#endif 	/* __IMGDFModule_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "d3d11.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_MGDF_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d2d1_1.h"
#ifdef __cplusplus
extern "C" {
#endif




extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0000_v0_0_s_ifspec;

#ifndef __IMGDFDebug_INTERFACE_DEFINED__
#define __IMGDFDebug_INTERFACE_DEFINED__

/* interface IMGDFDebug */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D69DC4E5-238B-4427-AF03-628818464E41")
    IMGDFDebug : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Set( 
            /* [string][in] */ const small *section,
            /* [in] */ const small *key,
            /* [string][in] */ const small *value) = 0;
        
        virtual void STDMETHODCALLTYPE Clear( 
            /* [in] */ const small *section,
            /* [in] */ const small *key) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsShown( void) = 0;
        
        virtual void STDMETHODCALLTYPE ToggleShown( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFDebug * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFDebug * This);
        
        void ( STDMETHODCALLTYPE *Set )( 
            IMGDFDebug * This,
            /* [string][in] */ const small *section,
            /* [in] */ const small *key,
            /* [string][in] */ const small *value);
        
        void ( STDMETHODCALLTYPE *Clear )( 
            IMGDFDebug * This,
            /* [in] */ const small *section,
            /* [in] */ const small *key);
        
        BOOL ( STDMETHODCALLTYPE *IsShown )( 
            IMGDFDebug * This);
        
        void ( STDMETHODCALLTYPE *ToggleShown )( 
            IMGDFDebug * This);
        
        END_INTERFACE
    } IMGDFDebugVtbl;

    interface IMGDFDebug
    {
        CONST_VTBL struct IMGDFDebugVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFDebug_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFDebug_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFDebug_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFDebug_Set(This,section,key,value)	\
    ( (This)->lpVtbl -> Set(This,section,key,value) ) 

#define IMGDFDebug_Clear(This,section,key)	\
    ( (This)->lpVtbl -> Clear(This,section,key) ) 

#define IMGDFDebug_IsShown(This)	\
    ( (This)->lpVtbl -> IsShown(This) ) 

#define IMGDFDebug_ToggleShown(This)	\
    ( (This)->lpVtbl -> ToggleShown(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFDebug_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_MGDF_0000_0001 */
/* [local] */ 

typedef struct MGDFVersion
    {
    INT32 Major;
    INT32 Minor;
    INT32 Build;
    INT32 Revision;
    } 	MGDFVersion;



extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0001_v0_0_s_ifspec;

#ifndef __IMGDFPreferenceSet_INTERFACE_DEFINED__
#define __IMGDFPreferenceSet_INTERFACE_DEFINED__

/* interface IMGDFPreferenceSet */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFPreferenceSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("167C7A2B-AA85-493B-842A-534A14669371")
    IMGDFPreferenceSet : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFPreferenceSetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFPreferenceSet * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFPreferenceSet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFPreferenceSet * This);
        
        END_INTERFACE
    } IMGDFPreferenceSetVtbl;

    interface IMGDFPreferenceSet
    {
        CONST_VTBL struct IMGDFPreferenceSetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFPreferenceSet_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFPreferenceSet_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFPreferenceSet_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFPreferenceSet_INTERFACE_DEFINED__ */


#ifndef __IMGDFGame_INTERFACE_DEFINED__
#define __IMGDFGame_INTERFACE_DEFINED__

/* interface IMGDFGame */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFGame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8BC6BFEF-09BC-4954-AF2F-5D2619F2CEEE")
    IMGDFGame : public IUnknown
    {
    public:
        virtual const small *STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual const small *STDMETHODCALLTYPE GetUid( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetVersion( 
            /* [out][in] */ MGDFVersion *__MIDL__IMGDFGame0000) = 0;
        
        virtual BOOL STDMETHODCALLTYPE HasPreference( 
            /* [in] */ const small *name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreference( 
            /* [in] */ const small *name,
            /* [out][in] */ small *value,
            /* [out][in] */ UINT64 *size) = 0;
        
        virtual void STDMETHODCALLTYPE SetPreference( 
            /* [in] */ const small *name,
            /* [in] */ const small *value) = 0;
        
        virtual void STDMETHODCALLTYPE SetPreferences( 
            /* [in] */ IMGDFPreferenceSet *preferences) = 0;
        
        virtual void STDMETHODCALLTYPE SavePreferences( void) = 0;
        
        virtual void STDMETHODCALLTYPE ResetPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFGameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFGame * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFGame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFGame * This);
        
        const small *( STDMETHODCALLTYPE *GetName )( 
            IMGDFGame * This);
        
        const small *( STDMETHODCALLTYPE *GetUid )( 
            IMGDFGame * This);
        
        void ( STDMETHODCALLTYPE *GetVersion )( 
            IMGDFGame * This,
            /* [out][in] */ MGDFVersion *__MIDL__IMGDFGame0000);
        
        BOOL ( STDMETHODCALLTYPE *HasPreference )( 
            IMGDFGame * This,
            /* [in] */ const small *name);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreference )( 
            IMGDFGame * This,
            /* [in] */ const small *name,
            /* [out][in] */ small *value,
            /* [out][in] */ UINT64 *size);
        
        void ( STDMETHODCALLTYPE *SetPreference )( 
            IMGDFGame * This,
            /* [in] */ const small *name,
            /* [in] */ const small *value);
        
        void ( STDMETHODCALLTYPE *SetPreferences )( 
            IMGDFGame * This,
            /* [in] */ IMGDFPreferenceSet *preferences);
        
        void ( STDMETHODCALLTYPE *SavePreferences )( 
            IMGDFGame * This);
        
        void ( STDMETHODCALLTYPE *ResetPreferences )( 
            IMGDFGame * This);
        
        END_INTERFACE
    } IMGDFGameVtbl;

    interface IMGDFGame
    {
        CONST_VTBL struct IMGDFGameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFGame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFGame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFGame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFGame_GetName(This)	\
    ( (This)->lpVtbl -> GetName(This) ) 

#define IMGDFGame_GetUid(This)	\
    ( (This)->lpVtbl -> GetUid(This) ) 

#define IMGDFGame_GetVersion(This,__MIDL__IMGDFGame0000)	\
    ( (This)->lpVtbl -> GetVersion(This,__MIDL__IMGDFGame0000) ) 

#define IMGDFGame_HasPreference(This,name)	\
    ( (This)->lpVtbl -> HasPreference(This,name) ) 

#define IMGDFGame_GetPreference(This,name,value,size)	\
    ( (This)->lpVtbl -> GetPreference(This,name,value,size) ) 

#define IMGDFGame_SetPreference(This,name,value)	\
    ( (This)->lpVtbl -> SetPreference(This,name,value) ) 

#define IMGDFGame_SetPreferences(This,preferences)	\
    ( (This)->lpVtbl -> SetPreferences(This,preferences) ) 

#define IMGDFGame_SavePreferences(This)	\
    ( (This)->lpVtbl -> SavePreferences(This) ) 

#define IMGDFGame_ResetPreferences(This)	\
    ( (This)->lpVtbl -> ResetPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFGame_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_MGDF_0000_0003 */
/* [local] */ 

typedef 
enum MGDFMouse
    {
        MGDF_MOUSE_LEFT	= 0,
        MGDF_MOUSE_RIGHT	= ( MGDF_MOUSE_LEFT + 1 ) ,
        MGDF_MOUSE_MIDDLE	= ( MGDF_MOUSE_RIGHT + 1 ) 
    } 	MGDFMouse;

typedef 
enum MGDFGamepadButton
    {
        MGDF_GAMEPAD_DPAD_UP	= 0x1,
        MGDF_GAMEPAD_DPAD_DOWN	= 0x2,
        MGDF_GAMEPAD_DPAD_LEFT	= 0x4,
        MGDF_GAMEPAD_DPAD_RIGHT	= 0x8,
        MGDF_GAMEPAD_START	= 0x10,
        MGDF_GAMEPAD_BACK	= 0x20,
        MGDF_GAMEPAD_LEFT_THUMB	= 0x40,
        MGDF_GAMEPAD_RIGHT_THUMB	= 0x80,
        MGDF_GAMEPAD_LEFT_SHOULDER	= 0x100,
        MGDF_GAMEPAD_RIGHT_SHOULDER	= 0x200,
        MGDF_GAMEPAD_A	= 0x1000,
        MGDF_GAMEPAD_B	= 0x2000,
        MGDF_GAMEPAD_X	= 0x4000,
        MGDF_GAMEPAD_Y	= 0x8000
    } 	MGDFGamepadButton;



extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0003_v0_0_s_ifspec;

#ifndef __IMGDFGamepad_INTERFACE_DEFINED__
#define __IMGDFGamepad_INTERFACE_DEFINED__

/* interface IMGDFGamepad */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFGamepad;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("84C7C6BF-827E-4958-8C64-D7FA78D69D0B")
    IMGDFGamepad : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsConnected( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonDown( 
            MGDFGamepadButton button) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonUp( 
            MGDFGamepadButton button) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonPress( 
            MGDFGamepadButton button) = 0;
        
        virtual UINT8 STDMETHODCALLTYPE GetLeftTrigger( void) = 0;
        
        virtual UINT8 STDMETHODCALLTYPE GetRightTrigger( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetLeftThumbX( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetLeftThumbY( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetRightThumbX( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetRightThumbY( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetVibrationSpeed( 
            UINT16 left,
            UINT16 right) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFGamepadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFGamepad * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFGamepad * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFGamepad * This);
        
        BOOL ( STDMETHODCALLTYPE *IsConnected )( 
            IMGDFGamepad * This);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonDown )( 
            IMGDFGamepad * This,
            MGDFGamepadButton button);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonUp )( 
            IMGDFGamepad * This,
            MGDFGamepadButton button);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonPress )( 
            IMGDFGamepad * This,
            MGDFGamepadButton button);
        
        UINT8 ( STDMETHODCALLTYPE *GetLeftTrigger )( 
            IMGDFGamepad * This);
        
        UINT8 ( STDMETHODCALLTYPE *GetRightTrigger )( 
            IMGDFGamepad * This);
        
        INT32 ( STDMETHODCALLTYPE *GetLeftThumbX )( 
            IMGDFGamepad * This);
        
        INT32 ( STDMETHODCALLTYPE *GetLeftThumbY )( 
            IMGDFGamepad * This);
        
        INT32 ( STDMETHODCALLTYPE *GetRightThumbX )( 
            IMGDFGamepad * This);
        
        INT32 ( STDMETHODCALLTYPE *GetRightThumbY )( 
            IMGDFGamepad * This);
        
        void ( STDMETHODCALLTYPE *SetVibrationSpeed )( 
            IMGDFGamepad * This,
            UINT16 left,
            UINT16 right);
        
        END_INTERFACE
    } IMGDFGamepadVtbl;

    interface IMGDFGamepad
    {
        CONST_VTBL struct IMGDFGamepadVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFGamepad_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFGamepad_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFGamepad_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFGamepad_IsConnected(This)	\
    ( (This)->lpVtbl -> IsConnected(This) ) 

#define IMGDFGamepad_IsButtonDown(This,button)	\
    ( (This)->lpVtbl -> IsButtonDown(This,button) ) 

#define IMGDFGamepad_IsButtonUp(This,button)	\
    ( (This)->lpVtbl -> IsButtonUp(This,button) ) 

#define IMGDFGamepad_IsButtonPress(This,button)	\
    ( (This)->lpVtbl -> IsButtonPress(This,button) ) 

#define IMGDFGamepad_GetLeftTrigger(This)	\
    ( (This)->lpVtbl -> GetLeftTrigger(This) ) 

#define IMGDFGamepad_GetRightTrigger(This)	\
    ( (This)->lpVtbl -> GetRightTrigger(This) ) 

#define IMGDFGamepad_GetLeftThumbX(This)	\
    ( (This)->lpVtbl -> GetLeftThumbX(This) ) 

#define IMGDFGamepad_GetLeftThumbY(This)	\
    ( (This)->lpVtbl -> GetLeftThumbY(This) ) 

#define IMGDFGamepad_GetRightThumbX(This)	\
    ( (This)->lpVtbl -> GetRightThumbX(This) ) 

#define IMGDFGamepad_GetRightThumbY(This)	\
    ( (This)->lpVtbl -> GetRightThumbY(This) ) 

#define IMGDFGamepad_SetVibrationSpeed(This,left,right)	\
    ( (This)->lpVtbl -> SetVibrationSpeed(This,left,right) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFGamepad_INTERFACE_DEFINED__ */


#ifndef __IMGDFInputManager_INTERFACE_DEFINED__
#define __IMGDFInputManager_INTERFACE_DEFINED__

/* interface IMGDFInputManager */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFInputManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98EC499D-5359-4902-8BA9-6E36686E9898")
    IMGDFInputManager : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE ShowCursor( 
            BOOL show) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsKeyDown( 
            UINT16 key) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsKeyUp( 
            UINT16 key) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsKeyPress( 
            UINT16 key) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetMouseX( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetMouseY( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetMouseDX( void) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetMouseDY( void) = 0;
        
        virtual short STDMETHODCALLTYPE GetMouseDZ( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonDown( 
            MGDFMouse mouseButton) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonUp( 
            MGDFMouse mouseButton) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsButtonClicked( 
            MGDFMouse mouseButton) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetGamepadCount( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetGamepads( 
            /* [out] */ IMGDFGamepad **gamepads) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFInputManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFInputManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFInputManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFInputManager * This);
        
        void ( STDMETHODCALLTYPE *ShowCursor )( 
            IMGDFInputManager * This,
            BOOL show);
        
        BOOL ( STDMETHODCALLTYPE *IsKeyDown )( 
            IMGDFInputManager * This,
            UINT16 key);
        
        BOOL ( STDMETHODCALLTYPE *IsKeyUp )( 
            IMGDFInputManager * This,
            UINT16 key);
        
        BOOL ( STDMETHODCALLTYPE *IsKeyPress )( 
            IMGDFInputManager * This,
            UINT16 key);
        
        INT32 ( STDMETHODCALLTYPE *GetMouseX )( 
            IMGDFInputManager * This);
        
        INT32 ( STDMETHODCALLTYPE *GetMouseY )( 
            IMGDFInputManager * This);
        
        INT32 ( STDMETHODCALLTYPE *GetMouseDX )( 
            IMGDFInputManager * This);
        
        INT32 ( STDMETHODCALLTYPE *GetMouseDY )( 
            IMGDFInputManager * This);
        
        short ( STDMETHODCALLTYPE *GetMouseDZ )( 
            IMGDFInputManager * This);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonDown )( 
            IMGDFInputManager * This,
            MGDFMouse mouseButton);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonUp )( 
            IMGDFInputManager * This,
            MGDFMouse mouseButton);
        
        BOOL ( STDMETHODCALLTYPE *IsButtonClicked )( 
            IMGDFInputManager * This,
            MGDFMouse mouseButton);
        
        UINT64 ( STDMETHODCALLTYPE *GetGamepadCount )( 
            IMGDFInputManager * This);
        
        void ( STDMETHODCALLTYPE *GetGamepads )( 
            IMGDFInputManager * This,
            /* [out] */ IMGDFGamepad **gamepads);
        
        END_INTERFACE
    } IMGDFInputManagerVtbl;

    interface IMGDFInputManager
    {
        CONST_VTBL struct IMGDFInputManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFInputManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFInputManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFInputManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFInputManager_ShowCursor(This,show)	\
    ( (This)->lpVtbl -> ShowCursor(This,show) ) 

#define IMGDFInputManager_IsKeyDown(This,key)	\
    ( (This)->lpVtbl -> IsKeyDown(This,key) ) 

#define IMGDFInputManager_IsKeyUp(This,key)	\
    ( (This)->lpVtbl -> IsKeyUp(This,key) ) 

#define IMGDFInputManager_IsKeyPress(This,key)	\
    ( (This)->lpVtbl -> IsKeyPress(This,key) ) 

#define IMGDFInputManager_GetMouseX(This)	\
    ( (This)->lpVtbl -> GetMouseX(This) ) 

#define IMGDFInputManager_GetMouseY(This)	\
    ( (This)->lpVtbl -> GetMouseY(This) ) 

#define IMGDFInputManager_GetMouseDX(This)	\
    ( (This)->lpVtbl -> GetMouseDX(This) ) 

#define IMGDFInputManager_GetMouseDY(This)	\
    ( (This)->lpVtbl -> GetMouseDY(This) ) 

#define IMGDFInputManager_GetMouseDZ(This)	\
    ( (This)->lpVtbl -> GetMouseDZ(This) ) 

#define IMGDFInputManager_IsButtonDown(This,mouseButton)	\
    ( (This)->lpVtbl -> IsButtonDown(This,mouseButton) ) 

#define IMGDFInputManager_IsButtonUp(This,mouseButton)	\
    ( (This)->lpVtbl -> IsButtonUp(This,mouseButton) ) 

#define IMGDFInputManager_IsButtonClicked(This,mouseButton)	\
    ( (This)->lpVtbl -> IsButtonClicked(This,mouseButton) ) 

#define IMGDFInputManager_GetGamepadCount(This)	\
    ( (This)->lpVtbl -> GetGamepadCount(This) ) 

#define IMGDFInputManager_GetGamepads(This,gamepads)	\
    ( (This)->lpVtbl -> GetGamepads(This,gamepads) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFInputManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_MGDF_0000_0005 */
/* [local] */ 

typedef 
enum MGDFLogLevel
    {
        MGDF_LOG_ERROR	= 0,
        MGDF_LOG_LOW	= ( MGDF_LOG_ERROR + 1 ) ,
        MGDF_LOG_MEDIUM	= ( MGDF_LOG_LOW + 1 ) ,
        MGDF_LOG_HIGH	= ( MGDF_LOG_MEDIUM + 1 ) 
    } 	MGDFLogLevel;



extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0005_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0005_v0_0_s_ifspec;

#ifndef __IMGDFLogger_INTERFACE_DEFINED__
#define __IMGDFLogger_INTERFACE_DEFINED__

/* interface IMGDFLogger */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFLogger;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B20785A5-B39E-494D-8D22-9FD0DAE1C4F9")
    IMGDFLogger : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE SetLoggingLevel( 
            MGDFLogLevel level) = 0;
        
        virtual MGDFLogLevel STDMETHODCALLTYPE GetLoggingLevel( void) = 0;
        
        virtual void STDMETHODCALLTYPE Log( 
            /* [in] */ const small *sender,
            /* [in] */ const small *message,
            MGDFLogLevel level) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFLoggerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFLogger * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFLogger * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFLogger * This);
        
        void ( STDMETHODCALLTYPE *SetLoggingLevel )( 
            IMGDFLogger * This,
            MGDFLogLevel level);
        
        MGDFLogLevel ( STDMETHODCALLTYPE *GetLoggingLevel )( 
            IMGDFLogger * This);
        
        void ( STDMETHODCALLTYPE *Log )( 
            IMGDFLogger * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message,
            MGDFLogLevel level);
        
        END_INTERFACE
    } IMGDFLoggerVtbl;

    interface IMGDFLogger
    {
        CONST_VTBL struct IMGDFLoggerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFLogger_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFLogger_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFLogger_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFLogger_SetLoggingLevel(This,level)	\
    ( (This)->lpVtbl -> SetLoggingLevel(This,level) ) 

#define IMGDFLogger_GetLoggingLevel(This)	\
    ( (This)->lpVtbl -> GetLoggingLevel(This) ) 

#define IMGDFLogger_Log(This,sender,message,level)	\
    ( (This)->lpVtbl -> Log(This,sender,message,level) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFLogger_INTERFACE_DEFINED__ */


#ifndef __IMGDFPerformanceCounter_INTERFACE_DEFINED__
#define __IMGDFPerformanceCounter_INTERFACE_DEFINED__

/* interface IMGDFPerformanceCounter */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFPerformanceCounter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E2E9FE7E-4D07-40FE-9858-8E583D73CB37")
    IMGDFPerformanceCounter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out][in] */ small *name,
            /* [out][in] */ UINT64 *length) = 0;
        
        virtual void STDMETHODCALLTYPE Begin( void) = 0;
        
        virtual void STDMETHODCALLTYPE End( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFPerformanceCounterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFPerformanceCounter * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFPerformanceCounter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFPerformanceCounter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IMGDFPerformanceCounter * This,
            /* [out][in] */ small *name,
            /* [out][in] */ UINT64 *length);
        
        void ( STDMETHODCALLTYPE *Begin )( 
            IMGDFPerformanceCounter * This);
        
        void ( STDMETHODCALLTYPE *End )( 
            IMGDFPerformanceCounter * This);
        
        END_INTERFACE
    } IMGDFPerformanceCounterVtbl;

    interface IMGDFPerformanceCounter
    {
        CONST_VTBL struct IMGDFPerformanceCounterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFPerformanceCounter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFPerformanceCounter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFPerformanceCounter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFPerformanceCounter_GetName(This,name,length)	\
    ( (This)->lpVtbl -> GetName(This,name,length) ) 

#define IMGDFPerformanceCounter_Begin(This)	\
    ( (This)->lpVtbl -> Begin(This) ) 

#define IMGDFPerformanceCounter_End(This)	\
    ( (This)->lpVtbl -> End(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFPerformanceCounter_INTERFACE_DEFINED__ */


#ifndef __IMGDFTimer_INTERFACE_DEFINED__
#define __IMGDFTimer_INTERFACE_DEFINED__

/* interface IMGDFTimer */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFTimer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4DA26ED2-514E-4CB2-B9D4-6AC4170DFDCE")
    IMGDFTimer : public IUnknown
    {
    public:
        virtual LARGE_INTEGER STDMETHODCALLTYPE GetCurrentTimeTicks( void) = 0;
        
        virtual LARGE_INTEGER STDMETHODCALLTYPE GetTimerFrequency( void) = 0;
        
        virtual double STDMETHODCALLTYPE ConvertDifferenceToSeconds( 
            LARGE_INTEGER newTime,
            LARGE_INTEGER oldTime) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFTimerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFTimer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFTimer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFTimer * This);
        
        LARGE_INTEGER ( STDMETHODCALLTYPE *GetCurrentTimeTicks )( 
            IMGDFTimer * This);
        
        LARGE_INTEGER ( STDMETHODCALLTYPE *GetTimerFrequency )( 
            IMGDFTimer * This);
        
        double ( STDMETHODCALLTYPE *ConvertDifferenceToSeconds )( 
            IMGDFTimer * This,
            LARGE_INTEGER newTime,
            LARGE_INTEGER oldTime);
        
        END_INTERFACE
    } IMGDFTimerVtbl;

    interface IMGDFTimer
    {
        CONST_VTBL struct IMGDFTimerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFTimer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFTimer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFTimer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFTimer_GetCurrentTimeTicks(This)	\
    ( (This)->lpVtbl -> GetCurrentTimeTicks(This) ) 

#define IMGDFTimer_GetTimerFrequency(This)	\
    ( (This)->lpVtbl -> GetTimerFrequency(This) ) 

#define IMGDFTimer_ConvertDifferenceToSeconds(This,newTime,oldTime)	\
    ( (This)->lpVtbl -> ConvertDifferenceToSeconds(This,newTime,oldTime) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFTimer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_MGDF_0000_0008 */
/* [local] */ 

typedef struct MGDFAdaptorMode
    {
    UINT32 Width;
    UINT32 Height;
    UINT32 RefreshRateNumerator;
    UINT32 RefreshRateDenominator;
    } 	MGDFAdaptorMode;

typedef struct MGDFFullScreenDesc
    {
    BOOL FullScreen;
    BOOL ExclusiveMode;
    } 	MGDFFullScreenDesc;



extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0008_v0_0_s_ifspec;

#ifndef __IMGDFRenderSettingsManager_INTERFACE_DEFINED__
#define __IMGDFRenderSettingsManager_INTERFACE_DEFINED__

/* interface IMGDFRenderSettingsManager */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFRenderSettingsManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BAF10DDF-1874-4299-A497-1F62C0691C8B")
    IMGDFRenderSettingsManager : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE GetVSync( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetVSync( 
            BOOL vsync) = 0;
        
        virtual void STDMETHODCALLTYPE GetFullscreen( 
            /* [out][in] */ MGDFFullScreenDesc *fullscreen) = 0;
        
        virtual void STDMETHODCALLTYPE SetFullscreen( 
            const MGDFFullScreenDesc *fullscreen) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetMultiSampleLevelCount( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetMultiSampleLevel( 
            UINT64 index,
            /* [out] */ UINT32 *level) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetBackBufferMultiSampleLevel( 
            UINT32 multisampleLevel) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetBackBufferMultiSampleLevel( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetCurrentMultiSampleLevel( 
            UINT32 multisampleLevel) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetCurrentMultiSampleLevel( 
            /* [out] */ UINT32 *quality) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetAdaptorModeCount( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetAdaptorMode( 
            UINT64 index,
            /* [out][in] */ MGDFAdaptorMode *mode) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetAdaptorModeFromDimensions( 
            UINT32 width,
            UINT32 height,
            /* [out][in] */ MGDFAdaptorMode *mode) = 0;
        
        virtual MGDFAdaptorMode *STDMETHODCALLTYPE GetCurrentAdaptorMode( 
            /* [out][in] */ MGDFAdaptorMode *mode) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetCurrentAdaptorMode( 
            /* [in] */ const MGDFAdaptorMode *mode) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetCurrentAdaptorModeToNative( 
            /* [out][in] */ MGDFAdaptorMode *mode) = 0;
        
        virtual void STDMETHODCALLTYPE SetWindowSize( 
            UINT32 width,
            UINT32 height) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetScreenX( void) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetScreenY( void) = 0;
        
        virtual void STDMETHODCALLTYPE ApplySettings( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetPreferences( 
            /* [out] */ IMGDFPreferenceSet **preferences) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFRenderSettingsManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFRenderSettingsManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFRenderSettingsManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFRenderSettingsManager * This);
        
        BOOL ( STDMETHODCALLTYPE *GetVSync )( 
            IMGDFRenderSettingsManager * This);
        
        void ( STDMETHODCALLTYPE *SetVSync )( 
            IMGDFRenderSettingsManager * This,
            BOOL vsync);
        
        void ( STDMETHODCALLTYPE *GetFullscreen )( 
            IMGDFRenderSettingsManager * This,
            /* [out][in] */ MGDFFullScreenDesc *fullscreen);
        
        void ( STDMETHODCALLTYPE *SetFullscreen )( 
            IMGDFRenderSettingsManager * This,
            const MGDFFullScreenDesc *fullscreen);
        
        UINT64 ( STDMETHODCALLTYPE *GetMultiSampleLevelCount )( 
            IMGDFRenderSettingsManager * This);
        
        BOOL ( STDMETHODCALLTYPE *GetMultiSampleLevel )( 
            IMGDFRenderSettingsManager * This,
            UINT64 index,
            /* [out] */ UINT32 *level);
        
        BOOL ( STDMETHODCALLTYPE *SetBackBufferMultiSampleLevel )( 
            IMGDFRenderSettingsManager * This,
            UINT32 multisampleLevel);
        
        UINT32 ( STDMETHODCALLTYPE *GetBackBufferMultiSampleLevel )( 
            IMGDFRenderSettingsManager * This);
        
        BOOL ( STDMETHODCALLTYPE *SetCurrentMultiSampleLevel )( 
            IMGDFRenderSettingsManager * This,
            UINT32 multisampleLevel);
        
        UINT32 ( STDMETHODCALLTYPE *GetCurrentMultiSampleLevel )( 
            IMGDFRenderSettingsManager * This,
            /* [out] */ UINT32 *quality);
        
        UINT64 ( STDMETHODCALLTYPE *GetAdaptorModeCount )( 
            IMGDFRenderSettingsManager * This);
        
        BOOL ( STDMETHODCALLTYPE *GetAdaptorMode )( 
            IMGDFRenderSettingsManager * This,
            UINT64 index,
            /* [out][in] */ MGDFAdaptorMode *mode);
        
        BOOL ( STDMETHODCALLTYPE *GetAdaptorModeFromDimensions )( 
            IMGDFRenderSettingsManager * This,
            UINT32 width,
            UINT32 height,
            /* [out][in] */ MGDFAdaptorMode *mode);
        
        MGDFAdaptorMode *( STDMETHODCALLTYPE *GetCurrentAdaptorMode )( 
            IMGDFRenderSettingsManager * This,
            /* [out][in] */ MGDFAdaptorMode *mode);
        
        BOOL ( STDMETHODCALLTYPE *SetCurrentAdaptorMode )( 
            IMGDFRenderSettingsManager * This,
            /* [in] */ const MGDFAdaptorMode *mode);
        
        BOOL ( STDMETHODCALLTYPE *SetCurrentAdaptorModeToNative )( 
            IMGDFRenderSettingsManager * This,
            /* [out][in] */ MGDFAdaptorMode *mode);
        
        void ( STDMETHODCALLTYPE *SetWindowSize )( 
            IMGDFRenderSettingsManager * This,
            UINT32 width,
            UINT32 height);
        
        UINT32 ( STDMETHODCALLTYPE *GetScreenX )( 
            IMGDFRenderSettingsManager * This);
        
        UINT32 ( STDMETHODCALLTYPE *GetScreenY )( 
            IMGDFRenderSettingsManager * This);
        
        void ( STDMETHODCALLTYPE *ApplySettings )( 
            IMGDFRenderSettingsManager * This);
        
        void ( STDMETHODCALLTYPE *GetPreferences )( 
            IMGDFRenderSettingsManager * This,
            /* [out] */ IMGDFPreferenceSet **preferences);
        
        END_INTERFACE
    } IMGDFRenderSettingsManagerVtbl;

    interface IMGDFRenderSettingsManager
    {
        CONST_VTBL struct IMGDFRenderSettingsManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFRenderSettingsManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFRenderSettingsManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFRenderSettingsManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFRenderSettingsManager_GetVSync(This)	\
    ( (This)->lpVtbl -> GetVSync(This) ) 

#define IMGDFRenderSettingsManager_SetVSync(This,vsync)	\
    ( (This)->lpVtbl -> SetVSync(This,vsync) ) 

#define IMGDFRenderSettingsManager_GetFullscreen(This,fullscreen)	\
    ( (This)->lpVtbl -> GetFullscreen(This,fullscreen) ) 

#define IMGDFRenderSettingsManager_SetFullscreen(This,fullscreen)	\
    ( (This)->lpVtbl -> SetFullscreen(This,fullscreen) ) 

#define IMGDFRenderSettingsManager_GetMultiSampleLevelCount(This)	\
    ( (This)->lpVtbl -> GetMultiSampleLevelCount(This) ) 

#define IMGDFRenderSettingsManager_GetMultiSampleLevel(This,index,level)	\
    ( (This)->lpVtbl -> GetMultiSampleLevel(This,index,level) ) 

#define IMGDFRenderSettingsManager_SetBackBufferMultiSampleLevel(This,multisampleLevel)	\
    ( (This)->lpVtbl -> SetBackBufferMultiSampleLevel(This,multisampleLevel) ) 

#define IMGDFRenderSettingsManager_GetBackBufferMultiSampleLevel(This)	\
    ( (This)->lpVtbl -> GetBackBufferMultiSampleLevel(This) ) 

#define IMGDFRenderSettingsManager_SetCurrentMultiSampleLevel(This,multisampleLevel)	\
    ( (This)->lpVtbl -> SetCurrentMultiSampleLevel(This,multisampleLevel) ) 

#define IMGDFRenderSettingsManager_GetCurrentMultiSampleLevel(This,quality)	\
    ( (This)->lpVtbl -> GetCurrentMultiSampleLevel(This,quality) ) 

#define IMGDFRenderSettingsManager_GetAdaptorModeCount(This)	\
    ( (This)->lpVtbl -> GetAdaptorModeCount(This) ) 

#define IMGDFRenderSettingsManager_GetAdaptorMode(This,index,mode)	\
    ( (This)->lpVtbl -> GetAdaptorMode(This,index,mode) ) 

#define IMGDFRenderSettingsManager_GetAdaptorModeFromDimensions(This,width,height,mode)	\
    ( (This)->lpVtbl -> GetAdaptorModeFromDimensions(This,width,height,mode) ) 

#define IMGDFRenderSettingsManager_GetCurrentAdaptorMode(This,mode)	\
    ( (This)->lpVtbl -> GetCurrentAdaptorMode(This,mode) ) 

#define IMGDFRenderSettingsManager_SetCurrentAdaptorMode(This,mode)	\
    ( (This)->lpVtbl -> SetCurrentAdaptorMode(This,mode) ) 

#define IMGDFRenderSettingsManager_SetCurrentAdaptorModeToNative(This,mode)	\
    ( (This)->lpVtbl -> SetCurrentAdaptorModeToNative(This,mode) ) 

#define IMGDFRenderSettingsManager_SetWindowSize(This,width,height)	\
    ( (This)->lpVtbl -> SetWindowSize(This,width,height) ) 

#define IMGDFRenderSettingsManager_GetScreenX(This)	\
    ( (This)->lpVtbl -> GetScreenX(This) ) 

#define IMGDFRenderSettingsManager_GetScreenY(This)	\
    ( (This)->lpVtbl -> GetScreenY(This) ) 

#define IMGDFRenderSettingsManager_ApplySettings(This)	\
    ( (This)->lpVtbl -> ApplySettings(This) ) 

#define IMGDFRenderSettingsManager_GetPreferences(This,preferences)	\
    ( (This)->lpVtbl -> GetPreferences(This,preferences) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFRenderSettingsManager_INTERFACE_DEFINED__ */


#ifndef __IMGDFFileReader_INTERFACE_DEFINED__
#define __IMGDFFileReader_INTERFACE_DEFINED__

/* interface IMGDFFileReader */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFFileReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C07D9A2-D197-4CDC-9DF6-3D244B854FB1")
    IMGDFFileReader : public IUnknown
    {
    public:
        virtual UINT32 STDMETHODCALLTYPE Read( 
            /* [out] */ void *buffer,
            UINT32 length) = 0;
        
        virtual void STDMETHODCALLTYPE SetPosition( 
            INT64 pos) = 0;
        
        virtual INT64 STDMETHODCALLTYPE GetPosition( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE EndOfFile( void) = 0;
        
        virtual INT64 STDMETHODCALLTYPE GetSize( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFFileReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFFileReader * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFFileReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFFileReader * This);
        
        UINT32 ( STDMETHODCALLTYPE *Read )( 
            IMGDFFileReader * This,
            /* [out] */ void *buffer,
            UINT32 length);
        
        void ( STDMETHODCALLTYPE *SetPosition )( 
            IMGDFFileReader * This,
            INT64 pos);
        
        INT64 ( STDMETHODCALLTYPE *GetPosition )( 
            IMGDFFileReader * This);
        
        BOOL ( STDMETHODCALLTYPE *EndOfFile )( 
            IMGDFFileReader * This);
        
        INT64 ( STDMETHODCALLTYPE *GetSize )( 
            IMGDFFileReader * This);
        
        END_INTERFACE
    } IMGDFFileReaderVtbl;

    interface IMGDFFileReader
    {
        CONST_VTBL struct IMGDFFileReaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFFileReader_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFFileReader_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFFileReader_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFFileReader_Read(This,buffer,length)	\
    ( (This)->lpVtbl -> Read(This,buffer,length) ) 

#define IMGDFFileReader_SetPosition(This,pos)	\
    ( (This)->lpVtbl -> SetPosition(This,pos) ) 

#define IMGDFFileReader_GetPosition(This)	\
    ( (This)->lpVtbl -> GetPosition(This) ) 

#define IMGDFFileReader_EndOfFile(This)	\
    ( (This)->lpVtbl -> EndOfFile(This) ) 

#define IMGDFFileReader_GetSize(This)	\
    ( (This)->lpVtbl -> GetSize(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFFileReader_INTERFACE_DEFINED__ */


#ifndef __IMGDFFile_INTERFACE_DEFINED__
#define __IMGDFFile_INTERFACE_DEFINED__

/* interface IMGDFFile */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFFile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7EB85F59-65C6-45AC-B505-F263E1771C7F")
    IMGDFFile : public IUnknown
    {
    public:
        virtual const wchar_t *STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetParent( 
            IMGDFFile **parent) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetChild( 
            /* [in] */ const wchar_t *name,
            /* [out] */ IMGDFFile **child) = 0;
        
        virtual void STDMETHODCALLTYPE GetAllChildren( 
            /* [out] */ IMGDFFile **childBuffer) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetChildCount( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsFolder( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsOpen( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [out] */ IMGDFFileReader **reader) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsArchive( void) = 0;
        
        virtual const wchar_t *STDMETHODCALLTYPE GetArchiveName( void) = 0;
        
        virtual const wchar_t *STDMETHODCALLTYPE GetPhysicalPath( void) = 0;
        
        virtual const wchar_t *STDMETHODCALLTYPE GetLogicalPath( void) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetLastWriteTime( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFFile * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFFile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFFile * This);
        
        const wchar_t *( STDMETHODCALLTYPE *GetName )( 
            IMGDFFile * This);
        
        BOOL ( STDMETHODCALLTYPE *GetParent )( 
            IMGDFFile * This,
            IMGDFFile **parent);
        
        BOOL ( STDMETHODCALLTYPE *GetChild )( 
            IMGDFFile * This,
            /* [in] */ const wchar_t *name,
            /* [out] */ IMGDFFile **child);
        
        void ( STDMETHODCALLTYPE *GetAllChildren )( 
            IMGDFFile * This,
            /* [out] */ IMGDFFile **childBuffer);
        
        UINT64 ( STDMETHODCALLTYPE *GetChildCount )( 
            IMGDFFile * This);
        
        BOOL ( STDMETHODCALLTYPE *IsFolder )( 
            IMGDFFile * This);
        
        BOOL ( STDMETHODCALLTYPE *IsOpen )( 
            IMGDFFile * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IMGDFFile * This,
            /* [out] */ IMGDFFileReader **reader);
        
        BOOL ( STDMETHODCALLTYPE *IsArchive )( 
            IMGDFFile * This);
        
        const wchar_t *( STDMETHODCALLTYPE *GetArchiveName )( 
            IMGDFFile * This);
        
        const wchar_t *( STDMETHODCALLTYPE *GetPhysicalPath )( 
            IMGDFFile * This);
        
        const wchar_t *( STDMETHODCALLTYPE *GetLogicalPath )( 
            IMGDFFile * This);
        
        UINT64 ( STDMETHODCALLTYPE *GetLastWriteTime )( 
            IMGDFFile * This);
        
        END_INTERFACE
    } IMGDFFileVtbl;

    interface IMGDFFile
    {
        CONST_VTBL struct IMGDFFileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFFile_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFFile_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFFile_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFFile_GetName(This)	\
    ( (This)->lpVtbl -> GetName(This) ) 

#define IMGDFFile_GetParent(This,parent)	\
    ( (This)->lpVtbl -> GetParent(This,parent) ) 

#define IMGDFFile_GetChild(This,name,child)	\
    ( (This)->lpVtbl -> GetChild(This,name,child) ) 

#define IMGDFFile_GetAllChildren(This,childBuffer)	\
    ( (This)->lpVtbl -> GetAllChildren(This,childBuffer) ) 

#define IMGDFFile_GetChildCount(This)	\
    ( (This)->lpVtbl -> GetChildCount(This) ) 

#define IMGDFFile_IsFolder(This)	\
    ( (This)->lpVtbl -> IsFolder(This) ) 

#define IMGDFFile_IsOpen(This)	\
    ( (This)->lpVtbl -> IsOpen(This) ) 

#define IMGDFFile_Open(This,reader)	\
    ( (This)->lpVtbl -> Open(This,reader) ) 

#define IMGDFFile_IsArchive(This)	\
    ( (This)->lpVtbl -> IsArchive(This) ) 

#define IMGDFFile_GetArchiveName(This)	\
    ( (This)->lpVtbl -> GetArchiveName(This) ) 

#define IMGDFFile_GetPhysicalPath(This)	\
    ( (This)->lpVtbl -> GetPhysicalPath(This) ) 

#define IMGDFFile_GetLogicalPath(This)	\
    ( (This)->lpVtbl -> GetLogicalPath(This) ) 

#define IMGDFFile_GetLastWriteTime(This)	\
    ( (This)->lpVtbl -> GetLastWriteTime(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFFile_INTERFACE_DEFINED__ */


#ifndef __IMGDFArchiveHandler_INTERFACE_DEFINED__
#define __IMGDFArchiveHandler_INTERFACE_DEFINED__

/* interface IMGDFArchiveHandler */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFArchiveHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09A21A95-9E64-4CCE-8CA3-CD938D94DD4D")
    IMGDFArchiveHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MapArchive( 
            /* [in] */ const wchar_t *name,
            /* [in] */ const wchar_t *physicalPath,
            /* [in] */ IMGDFFile *parent,
            /* [out] */ IMGDFFile **root) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsArchive( 
            /* [in] */ const wchar_t *physicalPath) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFArchiveHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFArchiveHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFArchiveHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFArchiveHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *MapArchive )( 
            IMGDFArchiveHandler * This,
            /* [in] */ const wchar_t *name,
            /* [in] */ const wchar_t *physicalPath,
            /* [in] */ IMGDFFile *parent,
            /* [out] */ IMGDFFile **root);
        
        BOOL ( STDMETHODCALLTYPE *IsArchive )( 
            IMGDFArchiveHandler * This,
            /* [in] */ const wchar_t *physicalPath);
        
        END_INTERFACE
    } IMGDFArchiveHandlerVtbl;

    interface IMGDFArchiveHandler
    {
        CONST_VTBL struct IMGDFArchiveHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFArchiveHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFArchiveHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFArchiveHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFArchiveHandler_MapArchive(This,name,physicalPath,parent,root)	\
    ( (This)->lpVtbl -> MapArchive(This,name,physicalPath,parent,root) ) 

#define IMGDFArchiveHandler_IsArchive(This,physicalPath)	\
    ( (This)->lpVtbl -> IsArchive(This,physicalPath) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFArchiveHandler_INTERFACE_DEFINED__ */


#ifndef __IMGDFVirtualFileSystem_INTERFACE_DEFINED__
#define __IMGDFVirtualFileSystem_INTERFACE_DEFINED__

/* interface IMGDFVirtualFileSystem */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFVirtualFileSystem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("235AB71E-BAE5-4ACB-BA98-82086378C939")
    IMGDFVirtualFileSystem : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE GetFile( 
            /* [in] */ const wchar_t *logicalPath,
            /* [out] */ IMGDFFile **file) = 0;
        
        virtual void STDMETHODCALLTYPE GetRoot( 
            /* [out] */ IMGDFFile **root) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFVirtualFileSystemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFVirtualFileSystem * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFVirtualFileSystem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFVirtualFileSystem * This);
        
        BOOL ( STDMETHODCALLTYPE *GetFile )( 
            IMGDFVirtualFileSystem * This,
            /* [in] */ const wchar_t *logicalPath,
            /* [out] */ IMGDFFile **file);
        
        void ( STDMETHODCALLTYPE *GetRoot )( 
            IMGDFVirtualFileSystem * This,
            /* [out] */ IMGDFFile **root);
        
        END_INTERFACE
    } IMGDFVirtualFileSystemVtbl;

    interface IMGDFVirtualFileSystem
    {
        CONST_VTBL struct IMGDFVirtualFileSystemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFVirtualFileSystem_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFVirtualFileSystem_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFVirtualFileSystem_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFVirtualFileSystem_GetFile(This,logicalPath,file)	\
    ( (This)->lpVtbl -> GetFile(This,logicalPath,file) ) 

#define IMGDFVirtualFileSystem_GetRoot(This,root)	\
    ( (This)->lpVtbl -> GetRoot(This,root) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFVirtualFileSystem_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_MGDF_0000_0013 */
/* [local] */ 

typedef struct MGDFSoundPosition
    {
    float x;
    float y;
    float z;
    } 	MGDFSoundPosition;



extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0013_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_MGDF_0000_0013_v0_0_s_ifspec;

#ifndef __IMGDFSound_INTERFACE_DEFINED__
#define __IMGDFSound_INTERFACE_DEFINED__

/* interface IMGDFSound */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFSound;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBCC1B73-815F-4870-BC80-67A5379E65FC")
    IMGDFSound : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out][in] */ wchar_t *name,
            /* [out][in] */ UINT64 *length) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetPosition( 
            MGDFSoundPosition *position) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetVelocity( 
            MGDFSoundPosition *velocity) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetPosition( 
            MGDFSoundPosition *position) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetVelocity( 
            MGDFSoundPosition *velocity) = 0;
        
        virtual float STDMETHODCALLTYPE GetInnerRange( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetInnerRange( 
            float innerRange) = 0;
        
        virtual float STDMETHODCALLTYPE GetOuterRange( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetOuterRange( 
            float outerRange) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetSourceRelative( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetSourceRelative( 
            BOOL sourceRelative) = 0;
        
        virtual float STDMETHODCALLTYPE GetVolume( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetVolume( 
            float volume) = 0;
        
        virtual float STDMETHODCALLTYPE GetPitch( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetPitch( 
            float pitch) = 0;
        
        virtual void STDMETHODCALLTYPE SetPriority( 
            INT32 priority) = 0;
        
        virtual INT32 STDMETHODCALLTYPE GetPriority( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetLooping( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetLooping( 
            BOOL looping) = 0;
        
        virtual void STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual void STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual void STDMETHODCALLTYPE Play( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsStopped( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsPaused( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsActive( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFSoundVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFSound * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFSound * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFSound * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IMGDFSound * This,
            /* [out][in] */ wchar_t *name,
            /* [out][in] */ UINT64 *length);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetPosition )( 
            IMGDFSound * This,
            MGDFSoundPosition *position);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetVelocity )( 
            IMGDFSound * This,
            MGDFSoundPosition *velocity);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetPosition )( 
            IMGDFSound * This,
            MGDFSoundPosition *position);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetVelocity )( 
            IMGDFSound * This,
            MGDFSoundPosition *velocity);
        
        float ( STDMETHODCALLTYPE *GetInnerRange )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetInnerRange )( 
            IMGDFSound * This,
            float innerRange);
        
        float ( STDMETHODCALLTYPE *GetOuterRange )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetOuterRange )( 
            IMGDFSound * This,
            float outerRange);
        
        BOOL ( STDMETHODCALLTYPE *GetSourceRelative )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetSourceRelative )( 
            IMGDFSound * This,
            BOOL sourceRelative);
        
        float ( STDMETHODCALLTYPE *GetVolume )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetVolume )( 
            IMGDFSound * This,
            float volume);
        
        float ( STDMETHODCALLTYPE *GetPitch )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetPitch )( 
            IMGDFSound * This,
            float pitch);
        
        void ( STDMETHODCALLTYPE *SetPriority )( 
            IMGDFSound * This,
            INT32 priority);
        
        INT32 ( STDMETHODCALLTYPE *GetPriority )( 
            IMGDFSound * This);
        
        BOOL ( STDMETHODCALLTYPE *GetLooping )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *SetLooping )( 
            IMGDFSound * This,
            BOOL looping);
        
        void ( STDMETHODCALLTYPE *Stop )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *Pause )( 
            IMGDFSound * This);
        
        void ( STDMETHODCALLTYPE *Play )( 
            IMGDFSound * This);
        
        BOOL ( STDMETHODCALLTYPE *IsStopped )( 
            IMGDFSound * This);
        
        BOOL ( STDMETHODCALLTYPE *IsPaused )( 
            IMGDFSound * This);
        
        BOOL ( STDMETHODCALLTYPE *IsPlaying )( 
            IMGDFSound * This);
        
        BOOL ( STDMETHODCALLTYPE *IsActive )( 
            IMGDFSound * This);
        
        END_INTERFACE
    } IMGDFSoundVtbl;

    interface IMGDFSound
    {
        CONST_VTBL struct IMGDFSoundVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFSound_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFSound_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFSound_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFSound_GetName(This,name,length)	\
    ( (This)->lpVtbl -> GetName(This,name,length) ) 

#define IMGDFSound_GetPosition(This,position)	\
    ( (This)->lpVtbl -> GetPosition(This,position) ) 

#define IMGDFSound_GetVelocity(This,velocity)	\
    ( (This)->lpVtbl -> GetVelocity(This,velocity) ) 

#define IMGDFSound_SetPosition(This,position)	\
    ( (This)->lpVtbl -> SetPosition(This,position) ) 

#define IMGDFSound_SetVelocity(This,velocity)	\
    ( (This)->lpVtbl -> SetVelocity(This,velocity) ) 

#define IMGDFSound_GetInnerRange(This)	\
    ( (This)->lpVtbl -> GetInnerRange(This) ) 

#define IMGDFSound_SetInnerRange(This,innerRange)	\
    ( (This)->lpVtbl -> SetInnerRange(This,innerRange) ) 

#define IMGDFSound_GetOuterRange(This)	\
    ( (This)->lpVtbl -> GetOuterRange(This) ) 

#define IMGDFSound_SetOuterRange(This,outerRange)	\
    ( (This)->lpVtbl -> SetOuterRange(This,outerRange) ) 

#define IMGDFSound_GetSourceRelative(This)	\
    ( (This)->lpVtbl -> GetSourceRelative(This) ) 

#define IMGDFSound_SetSourceRelative(This,sourceRelative)	\
    ( (This)->lpVtbl -> SetSourceRelative(This,sourceRelative) ) 

#define IMGDFSound_GetVolume(This)	\
    ( (This)->lpVtbl -> GetVolume(This) ) 

#define IMGDFSound_SetVolume(This,volume)	\
    ( (This)->lpVtbl -> SetVolume(This,volume) ) 

#define IMGDFSound_GetPitch(This)	\
    ( (This)->lpVtbl -> GetPitch(This) ) 

#define IMGDFSound_SetPitch(This,pitch)	\
    ( (This)->lpVtbl -> SetPitch(This,pitch) ) 

#define IMGDFSound_SetPriority(This,priority)	\
    ( (This)->lpVtbl -> SetPriority(This,priority) ) 

#define IMGDFSound_GetPriority(This)	\
    ( (This)->lpVtbl -> GetPriority(This) ) 

#define IMGDFSound_GetLooping(This)	\
    ( (This)->lpVtbl -> GetLooping(This) ) 

#define IMGDFSound_SetLooping(This,looping)	\
    ( (This)->lpVtbl -> SetLooping(This,looping) ) 

#define IMGDFSound_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define IMGDFSound_Pause(This)	\
    ( (This)->lpVtbl -> Pause(This) ) 

#define IMGDFSound_Play(This)	\
    ( (This)->lpVtbl -> Play(This) ) 

#define IMGDFSound_IsStopped(This)	\
    ( (This)->lpVtbl -> IsStopped(This) ) 

#define IMGDFSound_IsPaused(This)	\
    ( (This)->lpVtbl -> IsPaused(This) ) 

#define IMGDFSound_IsPlaying(This)	\
    ( (This)->lpVtbl -> IsPlaying(This) ) 

#define IMGDFSound_IsActive(This)	\
    ( (This)->lpVtbl -> IsActive(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFSound_INTERFACE_DEFINED__ */


#ifndef __IMGDFSoundStream_INTERFACE_DEFINED__
#define __IMGDFSoundStream_INTERFACE_DEFINED__

/* interface IMGDFSoundStream */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFSoundStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F0555E6-B5C3-40C4-9399-F74699A38811")
    IMGDFSoundStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out][in] */ wchar_t *name,
            /* [out][in] */ UINT64 *length) = 0;
        
        virtual float STDMETHODCALLTYPE GetVolume( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetVolume( 
            float volume) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetPosition( void) = 0;
        
        virtual UINT32 STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual void STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual void STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsStopped( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsPaused( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFSoundStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFSoundStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFSoundStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFSoundStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IMGDFSoundStream * This,
            /* [out][in] */ wchar_t *name,
            /* [out][in] */ UINT64 *length);
        
        float ( STDMETHODCALLTYPE *GetVolume )( 
            IMGDFSoundStream * This);
        
        void ( STDMETHODCALLTYPE *SetVolume )( 
            IMGDFSoundStream * This,
            float volume);
        
        UINT32 ( STDMETHODCALLTYPE *GetPosition )( 
            IMGDFSoundStream * This);
        
        UINT32 ( STDMETHODCALLTYPE *GetLength )( 
            IMGDFSoundStream * This);
        
        void ( STDMETHODCALLTYPE *Stop )( 
            IMGDFSoundStream * This);
        
        void ( STDMETHODCALLTYPE *Pause )( 
            IMGDFSoundStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *Play )( 
            IMGDFSoundStream * This);
        
        BOOL ( STDMETHODCALLTYPE *IsStopped )( 
            IMGDFSoundStream * This);
        
        BOOL ( STDMETHODCALLTYPE *IsPaused )( 
            IMGDFSoundStream * This);
        
        BOOL ( STDMETHODCALLTYPE *IsPlaying )( 
            IMGDFSoundStream * This);
        
        END_INTERFACE
    } IMGDFSoundStreamVtbl;

    interface IMGDFSoundStream
    {
        CONST_VTBL struct IMGDFSoundStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFSoundStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFSoundStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFSoundStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFSoundStream_GetName(This,name,length)	\
    ( (This)->lpVtbl -> GetName(This,name,length) ) 

#define IMGDFSoundStream_GetVolume(This)	\
    ( (This)->lpVtbl -> GetVolume(This) ) 

#define IMGDFSoundStream_SetVolume(This,volume)	\
    ( (This)->lpVtbl -> SetVolume(This,volume) ) 

#define IMGDFSoundStream_GetPosition(This)	\
    ( (This)->lpVtbl -> GetPosition(This) ) 

#define IMGDFSoundStream_GetLength(This)	\
    ( (This)->lpVtbl -> GetLength(This) ) 

#define IMGDFSoundStream_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define IMGDFSoundStream_Pause(This)	\
    ( (This)->lpVtbl -> Pause(This) ) 

#define IMGDFSoundStream_Play(This)	\
    ( (This)->lpVtbl -> Play(This) ) 

#define IMGDFSoundStream_IsStopped(This)	\
    ( (This)->lpVtbl -> IsStopped(This) ) 

#define IMGDFSoundStream_IsPaused(This)	\
    ( (This)->lpVtbl -> IsPaused(This) ) 

#define IMGDFSoundStream_IsPlaying(This)	\
    ( (This)->lpVtbl -> IsPlaying(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFSoundStream_INTERFACE_DEFINED__ */


#ifndef __IMGDFSoundManager_INTERFACE_DEFINED__
#define __IMGDFSoundManager_INTERFACE_DEFINED__

/* interface IMGDFSoundManager */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFSoundManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1E14DC82-8588-4686-888A-149993E4AE9B")
    IMGDFSoundManager : public IUnknown
    {
    public:
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetListenerPosition( 
            MGDFSoundPosition *listener) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetListenerVelocity( 
            MGDFSoundPosition *velocity) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetListenerOrientationForward( 
            MGDFSoundPosition *orientationForward) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE GetListenerOrientationUp( 
            MGDFSoundPosition *orientationUp) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetListenerPosition( 
            MGDFSoundPosition *listener) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetListenerVelocity( 
            MGDFSoundPosition *velocity) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetListenerOrientationForward( 
            MGDFSoundPosition *orientationForward) = 0;
        
        virtual MGDFSoundPosition *STDMETHODCALLTYPE SetListenerOrientationUp( 
            MGDFSoundPosition *orientationUp) = 0;
        
        virtual float STDMETHODCALLTYPE GetSoundVolume( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetSoundVolume( 
            float volume) = 0;
        
        virtual float STDMETHODCALLTYPE GetStreamVolume( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetStreamVolume( 
            float volume) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetEnableAttenuation( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetEnableAttenuation( 
            BOOL enableAttenuation) = 0;
        
        virtual float STDMETHODCALLTYPE GetDopplerShiftFactor( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetDopplerShiftFactor( 
            float dopplerShiftFactor) = 0;
        
        virtual float STDMETHODCALLTYPE GetSpeedOfSound( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetSpeedOfSound( 
            float speedOfSound) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSound( 
            /* [in] */ IMGDFFile *file,
            INT32 priority,
            /* [out] */ IMGDFSound **sound) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSoundStream( 
            /* [in] */ IMGDFFile *file,
            /* [out] */ IMGDFSoundStream **stream) = 0;
        
        virtual void STDMETHODCALLTYPE GetPreferences( 
            /* [out] */ IMGDFPreferenceSet **preferences) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFSoundManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFSoundManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFSoundManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFSoundManager * This);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetListenerPosition )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *listener);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetListenerVelocity )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *velocity);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetListenerOrientationForward )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *orientationForward);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *GetListenerOrientationUp )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *orientationUp);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetListenerPosition )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *listener);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetListenerVelocity )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *velocity);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetListenerOrientationForward )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *orientationForward);
        
        MGDFSoundPosition *( STDMETHODCALLTYPE *SetListenerOrientationUp )( 
            IMGDFSoundManager * This,
            MGDFSoundPosition *orientationUp);
        
        float ( STDMETHODCALLTYPE *GetSoundVolume )( 
            IMGDFSoundManager * This);
        
        void ( STDMETHODCALLTYPE *SetSoundVolume )( 
            IMGDFSoundManager * This,
            float volume);
        
        float ( STDMETHODCALLTYPE *GetStreamVolume )( 
            IMGDFSoundManager * This);
        
        void ( STDMETHODCALLTYPE *SetStreamVolume )( 
            IMGDFSoundManager * This,
            float volume);
        
        BOOL ( STDMETHODCALLTYPE *GetEnableAttenuation )( 
            IMGDFSoundManager * This);
        
        void ( STDMETHODCALLTYPE *SetEnableAttenuation )( 
            IMGDFSoundManager * This,
            BOOL enableAttenuation);
        
        float ( STDMETHODCALLTYPE *GetDopplerShiftFactor )( 
            IMGDFSoundManager * This);
        
        void ( STDMETHODCALLTYPE *SetDopplerShiftFactor )( 
            IMGDFSoundManager * This,
            float dopplerShiftFactor);
        
        float ( STDMETHODCALLTYPE *GetSpeedOfSound )( 
            IMGDFSoundManager * This);
        
        void ( STDMETHODCALLTYPE *SetSpeedOfSound )( 
            IMGDFSoundManager * This,
            float speedOfSound);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSound )( 
            IMGDFSoundManager * This,
            /* [in] */ IMGDFFile *file,
            INT32 priority,
            /* [out] */ IMGDFSound **sound);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoundStream )( 
            IMGDFSoundManager * This,
            /* [in] */ IMGDFFile *file,
            /* [out] */ IMGDFSoundStream **stream);
        
        void ( STDMETHODCALLTYPE *GetPreferences )( 
            IMGDFSoundManager * This,
            /* [out] */ IMGDFPreferenceSet **preferences);
        
        END_INTERFACE
    } IMGDFSoundManagerVtbl;

    interface IMGDFSoundManager
    {
        CONST_VTBL struct IMGDFSoundManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFSoundManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFSoundManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFSoundManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFSoundManager_GetListenerPosition(This,listener)	\
    ( (This)->lpVtbl -> GetListenerPosition(This,listener) ) 

#define IMGDFSoundManager_GetListenerVelocity(This,velocity)	\
    ( (This)->lpVtbl -> GetListenerVelocity(This,velocity) ) 

#define IMGDFSoundManager_GetListenerOrientationForward(This,orientationForward)	\
    ( (This)->lpVtbl -> GetListenerOrientationForward(This,orientationForward) ) 

#define IMGDFSoundManager_GetListenerOrientationUp(This,orientationUp)	\
    ( (This)->lpVtbl -> GetListenerOrientationUp(This,orientationUp) ) 

#define IMGDFSoundManager_SetListenerPosition(This,listener)	\
    ( (This)->lpVtbl -> SetListenerPosition(This,listener) ) 

#define IMGDFSoundManager_SetListenerVelocity(This,velocity)	\
    ( (This)->lpVtbl -> SetListenerVelocity(This,velocity) ) 

#define IMGDFSoundManager_SetListenerOrientationForward(This,orientationForward)	\
    ( (This)->lpVtbl -> SetListenerOrientationForward(This,orientationForward) ) 

#define IMGDFSoundManager_SetListenerOrientationUp(This,orientationUp)	\
    ( (This)->lpVtbl -> SetListenerOrientationUp(This,orientationUp) ) 

#define IMGDFSoundManager_GetSoundVolume(This)	\
    ( (This)->lpVtbl -> GetSoundVolume(This) ) 

#define IMGDFSoundManager_SetSoundVolume(This,volume)	\
    ( (This)->lpVtbl -> SetSoundVolume(This,volume) ) 

#define IMGDFSoundManager_GetStreamVolume(This)	\
    ( (This)->lpVtbl -> GetStreamVolume(This) ) 

#define IMGDFSoundManager_SetStreamVolume(This,volume)	\
    ( (This)->lpVtbl -> SetStreamVolume(This,volume) ) 

#define IMGDFSoundManager_GetEnableAttenuation(This)	\
    ( (This)->lpVtbl -> GetEnableAttenuation(This) ) 

#define IMGDFSoundManager_SetEnableAttenuation(This,enableAttenuation)	\
    ( (This)->lpVtbl -> SetEnableAttenuation(This,enableAttenuation) ) 

#define IMGDFSoundManager_GetDopplerShiftFactor(This)	\
    ( (This)->lpVtbl -> GetDopplerShiftFactor(This) ) 

#define IMGDFSoundManager_SetDopplerShiftFactor(This,dopplerShiftFactor)	\
    ( (This)->lpVtbl -> SetDopplerShiftFactor(This,dopplerShiftFactor) ) 

#define IMGDFSoundManager_GetSpeedOfSound(This)	\
    ( (This)->lpVtbl -> GetSpeedOfSound(This) ) 

#define IMGDFSoundManager_SetSpeedOfSound(This,speedOfSound)	\
    ( (This)->lpVtbl -> SetSpeedOfSound(This,speedOfSound) ) 

#define IMGDFSoundManager_CreateSound(This,file,priority,sound)	\
    ( (This)->lpVtbl -> CreateSound(This,file,priority,sound) ) 

#define IMGDFSoundManager_CreateSoundStream(This,file,stream)	\
    ( (This)->lpVtbl -> CreateSoundStream(This,file,stream) ) 

#define IMGDFSoundManager_GetPreferences(This,preferences)	\
    ( (This)->lpVtbl -> GetPreferences(This,preferences) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFSoundManager_INTERFACE_DEFINED__ */


#ifndef __IMGDFStatisticsManager_INTERFACE_DEFINED__
#define __IMGDFStatisticsManager_INTERFACE_DEFINED__

/* interface IMGDFStatisticsManager */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFStatisticsManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F809B9CB-AE6B-43EE-8ACC-40BCB11014E1")
    IMGDFStatisticsManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SaveStatistic( 
            /* [in] */ const small *name,
            /* [in] */ const small *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFStatisticsManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFStatisticsManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFStatisticsManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFStatisticsManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SaveStatistic )( 
            IMGDFStatisticsManager * This,
            /* [in] */ const small *name,
            /* [in] */ const small *value);
        
        END_INTERFACE
    } IMGDFStatisticsManagerVtbl;

    interface IMGDFStatisticsManager
    {
        CONST_VTBL struct IMGDFStatisticsManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFStatisticsManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFStatisticsManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFStatisticsManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFStatisticsManager_SaveStatistic(This,name,value)	\
    ( (This)->lpVtbl -> SaveStatistic(This,name,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFStatisticsManager_INTERFACE_DEFINED__ */


#ifndef __IMGDFCommonHost_INTERFACE_DEFINED__
#define __IMGDFCommonHost_INTERFACE_DEFINED__

/* interface IMGDFCommonHost */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFCommonHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B6FDBA44-D098-4EF7-AF79-7368FB85AA6C")
    IMGDFCommonHost : public IMGDFLogger
    {
    public:
        virtual void STDMETHODCALLTYPE FatalError( 
            /* [in] */ const small *sender,
            /* [in] */ const small *message) = 0;
        
        virtual void STDMETHODCALLTYPE GetRenderSettings( 
            /* [out] */ IMGDFRenderSettingsManager **settings) = 0;
        
        virtual void STDMETHODCALLTYPE GetTimer( 
            /* [out] */ IMGDFTimer **timer) = 0;
        
        virtual const MGDFVersion *STDMETHODCALLTYPE GetMGDFVersion( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetVFS( 
            /* [out] */ IMGDFVirtualFileSystem **vfs) = 0;
        
        virtual ID3D11Device *STDMETHODCALLTYPE GetD3DDevice( void) = 0;
        
        virtual ID2D1Device *STDMETHODCALLTYPE GetD2DDevice( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetDebug( 
            /* [out] */ IMGDFDebug **debug) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCPUCounter( 
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFCommonHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFCommonHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFCommonHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFCommonHost * This);
        
        void ( STDMETHODCALLTYPE *SetLoggingLevel )( 
            IMGDFCommonHost * This,
            MGDFLogLevel level);
        
        MGDFLogLevel ( STDMETHODCALLTYPE *GetLoggingLevel )( 
            IMGDFCommonHost * This);
        
        void ( STDMETHODCALLTYPE *Log )( 
            IMGDFCommonHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message,
            MGDFLogLevel level);
        
        void ( STDMETHODCALLTYPE *FatalError )( 
            IMGDFCommonHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message);
        
        void ( STDMETHODCALLTYPE *GetRenderSettings )( 
            IMGDFCommonHost * This,
            /* [out] */ IMGDFRenderSettingsManager **settings);
        
        void ( STDMETHODCALLTYPE *GetTimer )( 
            IMGDFCommonHost * This,
            /* [out] */ IMGDFTimer **timer);
        
        const MGDFVersion *( STDMETHODCALLTYPE *GetMGDFVersion )( 
            IMGDFCommonHost * This);
        
        void ( STDMETHODCALLTYPE *GetVFS )( 
            IMGDFCommonHost * This,
            /* [out] */ IMGDFVirtualFileSystem **vfs);
        
        ID3D11Device *( STDMETHODCALLTYPE *GetD3DDevice )( 
            IMGDFCommonHost * This);
        
        ID2D1Device *( STDMETHODCALLTYPE *GetD2DDevice )( 
            IMGDFCommonHost * This);
        
        void ( STDMETHODCALLTYPE *GetDebug )( 
            IMGDFCommonHost * This,
            /* [out] */ IMGDFDebug **debug);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCPUCounter )( 
            IMGDFCommonHost * This,
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter);
        
        END_INTERFACE
    } IMGDFCommonHostVtbl;

    interface IMGDFCommonHost
    {
        CONST_VTBL struct IMGDFCommonHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFCommonHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFCommonHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFCommonHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFCommonHost_SetLoggingLevel(This,level)	\
    ( (This)->lpVtbl -> SetLoggingLevel(This,level) ) 

#define IMGDFCommonHost_GetLoggingLevel(This)	\
    ( (This)->lpVtbl -> GetLoggingLevel(This) ) 

#define IMGDFCommonHost_Log(This,sender,message,level)	\
    ( (This)->lpVtbl -> Log(This,sender,message,level) ) 


#define IMGDFCommonHost_FatalError(This,sender,message)	\
    ( (This)->lpVtbl -> FatalError(This,sender,message) ) 

#define IMGDFCommonHost_GetRenderSettings(This,settings)	\
    ( (This)->lpVtbl -> GetRenderSettings(This,settings) ) 

#define IMGDFCommonHost_GetTimer(This,timer)	\
    ( (This)->lpVtbl -> GetTimer(This,timer) ) 

#define IMGDFCommonHost_GetMGDFVersion(This)	\
    ( (This)->lpVtbl -> GetMGDFVersion(This) ) 

#define IMGDFCommonHost_GetVFS(This,vfs)	\
    ( (This)->lpVtbl -> GetVFS(This,vfs) ) 

#define IMGDFCommonHost_GetD3DDevice(This)	\
    ( (This)->lpVtbl -> GetD3DDevice(This) ) 

#define IMGDFCommonHost_GetD2DDevice(This)	\
    ( (This)->lpVtbl -> GetD2DDevice(This) ) 

#define IMGDFCommonHost_GetDebug(This,debug)	\
    ( (This)->lpVtbl -> GetDebug(This,debug) ) 

#define IMGDFCommonHost_CreateCPUCounter(This,name,counter)	\
    ( (This)->lpVtbl -> CreateCPUCounter(This,name,counter) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFCommonHost_INTERFACE_DEFINED__ */


#ifndef __IMGDFRenderHost_INTERFACE_DEFINED__
#define __IMGDFRenderHost_INTERFACE_DEFINED__

/* interface IMGDFRenderHost */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFRenderHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("514EBD97-3C53-4235-90E1-7AD3B7F517BD")
    IMGDFRenderHost : public IMGDFCommonHost
    {
    public:
        virtual ID3D11DeviceContext *STDMETHODCALLTYPE GetD3DImmediateContext( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetBackBufferRenderTarget( 
            ID2D1DeviceContext *context) = 0;
        
        virtual ID3D11Texture2D *STDMETHODCALLTYPE GetBackBuffer( void) = 0;
        
        virtual ID3D11Texture2D *STDMETHODCALLTYPE GetDepthStencilBuffer( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetBackBufferDescription( 
            D3D11_TEXTURE2D_DESC *backBufferDesc,
            D3D11_TEXTURE2D_DESC *depthStencilBufferDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateGPUCounter( 
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFRenderHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFRenderHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFRenderHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFRenderHost * This);
        
        void ( STDMETHODCALLTYPE *SetLoggingLevel )( 
            IMGDFRenderHost * This,
            MGDFLogLevel level);
        
        MGDFLogLevel ( STDMETHODCALLTYPE *GetLoggingLevel )( 
            IMGDFRenderHost * This);
        
        void ( STDMETHODCALLTYPE *Log )( 
            IMGDFRenderHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message,
            MGDFLogLevel level);
        
        void ( STDMETHODCALLTYPE *FatalError )( 
            IMGDFRenderHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message);
        
        void ( STDMETHODCALLTYPE *GetRenderSettings )( 
            IMGDFRenderHost * This,
            /* [out] */ IMGDFRenderSettingsManager **settings);
        
        void ( STDMETHODCALLTYPE *GetTimer )( 
            IMGDFRenderHost * This,
            /* [out] */ IMGDFTimer **timer);
        
        const MGDFVersion *( STDMETHODCALLTYPE *GetMGDFVersion )( 
            IMGDFRenderHost * This);
        
        void ( STDMETHODCALLTYPE *GetVFS )( 
            IMGDFRenderHost * This,
            /* [out] */ IMGDFVirtualFileSystem **vfs);
        
        ID3D11Device *( STDMETHODCALLTYPE *GetD3DDevice )( 
            IMGDFRenderHost * This);
        
        ID2D1Device *( STDMETHODCALLTYPE *GetD2DDevice )( 
            IMGDFRenderHost * This);
        
        void ( STDMETHODCALLTYPE *GetDebug )( 
            IMGDFRenderHost * This,
            /* [out] */ IMGDFDebug **debug);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCPUCounter )( 
            IMGDFRenderHost * This,
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter);
        
        ID3D11DeviceContext *( STDMETHODCALLTYPE *GetD3DImmediateContext )( 
            IMGDFRenderHost * This);
        
        BOOL ( STDMETHODCALLTYPE *SetBackBufferRenderTarget )( 
            IMGDFRenderHost * This,
            ID2D1DeviceContext *context);
        
        ID3D11Texture2D *( STDMETHODCALLTYPE *GetBackBuffer )( 
            IMGDFRenderHost * This);
        
        ID3D11Texture2D *( STDMETHODCALLTYPE *GetDepthStencilBuffer )( 
            IMGDFRenderHost * This);
        
        void ( STDMETHODCALLTYPE *GetBackBufferDescription )( 
            IMGDFRenderHost * This,
            D3D11_TEXTURE2D_DESC *backBufferDesc,
            D3D11_TEXTURE2D_DESC *depthStencilBufferDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGPUCounter )( 
            IMGDFRenderHost * This,
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter);
        
        END_INTERFACE
    } IMGDFRenderHostVtbl;

    interface IMGDFRenderHost
    {
        CONST_VTBL struct IMGDFRenderHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFRenderHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFRenderHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFRenderHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFRenderHost_SetLoggingLevel(This,level)	\
    ( (This)->lpVtbl -> SetLoggingLevel(This,level) ) 

#define IMGDFRenderHost_GetLoggingLevel(This)	\
    ( (This)->lpVtbl -> GetLoggingLevel(This) ) 

#define IMGDFRenderHost_Log(This,sender,message,level)	\
    ( (This)->lpVtbl -> Log(This,sender,message,level) ) 


#define IMGDFRenderHost_FatalError(This,sender,message)	\
    ( (This)->lpVtbl -> FatalError(This,sender,message) ) 

#define IMGDFRenderHost_GetRenderSettings(This,settings)	\
    ( (This)->lpVtbl -> GetRenderSettings(This,settings) ) 

#define IMGDFRenderHost_GetTimer(This,timer)	\
    ( (This)->lpVtbl -> GetTimer(This,timer) ) 

#define IMGDFRenderHost_GetMGDFVersion(This)	\
    ( (This)->lpVtbl -> GetMGDFVersion(This) ) 

#define IMGDFRenderHost_GetVFS(This,vfs)	\
    ( (This)->lpVtbl -> GetVFS(This,vfs) ) 

#define IMGDFRenderHost_GetD3DDevice(This)	\
    ( (This)->lpVtbl -> GetD3DDevice(This) ) 

#define IMGDFRenderHost_GetD2DDevice(This)	\
    ( (This)->lpVtbl -> GetD2DDevice(This) ) 

#define IMGDFRenderHost_GetDebug(This,debug)	\
    ( (This)->lpVtbl -> GetDebug(This,debug) ) 

#define IMGDFRenderHost_CreateCPUCounter(This,name,counter)	\
    ( (This)->lpVtbl -> CreateCPUCounter(This,name,counter) ) 


#define IMGDFRenderHost_GetD3DImmediateContext(This)	\
    ( (This)->lpVtbl -> GetD3DImmediateContext(This) ) 

#define IMGDFRenderHost_SetBackBufferRenderTarget(This,context)	\
    ( (This)->lpVtbl -> SetBackBufferRenderTarget(This,context) ) 

#define IMGDFRenderHost_GetBackBuffer(This)	\
    ( (This)->lpVtbl -> GetBackBuffer(This) ) 

#define IMGDFRenderHost_GetDepthStencilBuffer(This)	\
    ( (This)->lpVtbl -> GetDepthStencilBuffer(This) ) 

#define IMGDFRenderHost_GetBackBufferDescription(This,backBufferDesc,depthStencilBufferDesc)	\
    ( (This)->lpVtbl -> GetBackBufferDescription(This,backBufferDesc,depthStencilBufferDesc) ) 

#define IMGDFRenderHost_CreateGPUCounter(This,name,counter)	\
    ( (This)->lpVtbl -> CreateGPUCounter(This,name,counter) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFRenderHost_INTERFACE_DEFINED__ */


#ifndef __IMGDFPendingSave_INTERFACE_DEFINED__
#define __IMGDFPendingSave_INTERFACE_DEFINED__

/* interface IMGDFPendingSave */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFPendingSave;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("55333E24-25ED-452B-9CDC-9031A9584C59")
    IMGDFPendingSave : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSaveDataLocation( 
            /* [out][in] */ wchar_t *folder,
            /* [out][in] */ UINT64 *size) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFPendingSaveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFPendingSave * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFPendingSave * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFPendingSave * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSaveDataLocation )( 
            IMGDFPendingSave * This,
            /* [out][in] */ wchar_t *folder,
            /* [out][in] */ UINT64 *size);
        
        END_INTERFACE
    } IMGDFPendingSaveVtbl;

    interface IMGDFPendingSave
    {
        CONST_VTBL struct IMGDFPendingSaveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFPendingSave_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFPendingSave_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFPendingSave_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFPendingSave_GetSaveDataLocation(This,folder,size)	\
    ( (This)->lpVtbl -> GetSaveDataLocation(This,folder,size) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFPendingSave_INTERFACE_DEFINED__ */


#ifndef __IMGDFGameState_INTERFACE_DEFINED__
#define __IMGDFGameState_INTERFACE_DEFINED__

/* interface IMGDFGameState */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFGameState;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B34FC7A2-2F84-4FC7-B821-4AEDA8AB9F20")
    IMGDFGameState : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMetadata( 
            /* [in] */ const small *key,
            /* [out][in] */ small *value,
            /* [out][in] */ UINT64 *length) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMetadata( 
            /* [out][in] */ const small *key,
            /* [out][in] */ const small *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSaveDataLocation( 
            /* [out][in] */ wchar_t *folder,
            /* [out][in] */ UINT64 *size) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsNew( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetVersion( 
            MGDFVersion *version) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginSave( 
            /* [out] */ IMGDFPendingSave **pending) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFGameStateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFGameState * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFGameState * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFGameState * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IMGDFGameState * This,
            /* [in] */ const small *key,
            /* [out][in] */ small *value,
            /* [out][in] */ UINT64 *length);
        
        HRESULT ( STDMETHODCALLTYPE *SetMetadata )( 
            IMGDFGameState * This,
            /* [out][in] */ const small *key,
            /* [out][in] */ const small *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetSaveDataLocation )( 
            IMGDFGameState * This,
            /* [out][in] */ wchar_t *folder,
            /* [out][in] */ UINT64 *size);
        
        BOOL ( STDMETHODCALLTYPE *IsNew )( 
            IMGDFGameState * This);
        
        void ( STDMETHODCALLTYPE *GetVersion )( 
            IMGDFGameState * This,
            MGDFVersion *version);
        
        HRESULT ( STDMETHODCALLTYPE *BeginSave )( 
            IMGDFGameState * This,
            /* [out] */ IMGDFPendingSave **pending);
        
        END_INTERFACE
    } IMGDFGameStateVtbl;

    interface IMGDFGameState
    {
        CONST_VTBL struct IMGDFGameStateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFGameState_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFGameState_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFGameState_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFGameState_GetMetadata(This,key,value,length)	\
    ( (This)->lpVtbl -> GetMetadata(This,key,value,length) ) 

#define IMGDFGameState_SetMetadata(This,key,value)	\
    ( (This)->lpVtbl -> SetMetadata(This,key,value) ) 

#define IMGDFGameState_GetSaveDataLocation(This,folder,size)	\
    ( (This)->lpVtbl -> GetSaveDataLocation(This,folder,size) ) 

#define IMGDFGameState_IsNew(This)	\
    ( (This)->lpVtbl -> IsNew(This) ) 

#define IMGDFGameState_GetVersion(This,version)	\
    ( (This)->lpVtbl -> GetVersion(This,version) ) 

#define IMGDFGameState_BeginSave(This,pending)	\
    ( (This)->lpVtbl -> BeginSave(This,pending) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFGameState_INTERFACE_DEFINED__ */


#ifndef __IMGDFSaveManager_INTERFACE_DEFINED__
#define __IMGDFSaveManager_INTERFACE_DEFINED__

/* interface IMGDFSaveManager */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFSaveManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("58D65D58-358D-4999-B32C-F32CF0B0AC83")
    IMGDFSaveManager : public IUnknown
    {
    public:
        virtual UINT64 STDMETHODCALLTYPE GetSaveCount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSave( 
            UINT64 index,
            /* [out] */ IMGDFGameState **save) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteSave( 
            /* [out] */ IMGDFGameState *state) = 0;
        
        virtual void STDMETHODCALLTYPE CreateGameState( 
            /* [out] */ IMGDFGameState **save) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFSaveManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFSaveManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFSaveManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFSaveManager * This);
        
        UINT64 ( STDMETHODCALLTYPE *GetSaveCount )( 
            IMGDFSaveManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSave )( 
            IMGDFSaveManager * This,
            UINT64 index,
            /* [out] */ IMGDFGameState **save);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteSave )( 
            IMGDFSaveManager * This,
            /* [out] */ IMGDFGameState *state);
        
        void ( STDMETHODCALLTYPE *CreateGameState )( 
            IMGDFSaveManager * This,
            /* [out] */ IMGDFGameState **save);
        
        END_INTERFACE
    } IMGDFSaveManagerVtbl;

    interface IMGDFSaveManager
    {
        CONST_VTBL struct IMGDFSaveManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFSaveManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFSaveManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFSaveManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFSaveManager_GetSaveCount(This)	\
    ( (This)->lpVtbl -> GetSaveCount(This) ) 

#define IMGDFSaveManager_GetSave(This,index,save)	\
    ( (This)->lpVtbl -> GetSave(This,index,save) ) 

#define IMGDFSaveManager_DeleteSave(This,state)	\
    ( (This)->lpVtbl -> DeleteSave(This,state) ) 

#define IMGDFSaveManager_CreateGameState(This,save)	\
    ( (This)->lpVtbl -> CreateGameState(This,save) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFSaveManager_INTERFACE_DEFINED__ */


#ifndef __IMGDFSimHost_INTERFACE_DEFINED__
#define __IMGDFSimHost_INTERFACE_DEFINED__

/* interface IMGDFSimHost */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFSimHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("381AD5F1-8058-4739-992F-A5B551AA0E87")
    IMGDFSimHost : public IMGDFCommonHost
    {
    public:
        virtual void STDMETHODCALLTYPE GetSaves( 
            /* [out] */ IMGDFSaveManager **save) = 0;
        
        virtual void STDMETHODCALLTYPE GetGame( 
            /* [out] */ IMGDFGame **game) = 0;
        
        virtual void STDMETHODCALLTYPE GetSound( 
            /* [out] */ IMGDFSoundManager **manager) = 0;
        
        virtual void STDMETHODCALLTYPE GetStatistics( 
            /* [out] */ IMGDFStatisticsManager **statistics) = 0;
        
        virtual void STDMETHODCALLTYPE GetInput( 
            /* [out] */ IMGDFInputManager **manager) = 0;
        
        virtual void STDMETHODCALLTYPE ShutDown( void) = 0;
        
        virtual void STDMETHODCALLTYPE QueueShutDown( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFSimHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFSimHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFSimHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFSimHost * This);
        
        void ( STDMETHODCALLTYPE *SetLoggingLevel )( 
            IMGDFSimHost * This,
            MGDFLogLevel level);
        
        MGDFLogLevel ( STDMETHODCALLTYPE *GetLoggingLevel )( 
            IMGDFSimHost * This);
        
        void ( STDMETHODCALLTYPE *Log )( 
            IMGDFSimHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message,
            MGDFLogLevel level);
        
        void ( STDMETHODCALLTYPE *FatalError )( 
            IMGDFSimHost * This,
            /* [in] */ const small *sender,
            /* [in] */ const small *message);
        
        void ( STDMETHODCALLTYPE *GetRenderSettings )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFRenderSettingsManager **settings);
        
        void ( STDMETHODCALLTYPE *GetTimer )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFTimer **timer);
        
        const MGDFVersion *( STDMETHODCALLTYPE *GetMGDFVersion )( 
            IMGDFSimHost * This);
        
        void ( STDMETHODCALLTYPE *GetVFS )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFVirtualFileSystem **vfs);
        
        ID3D11Device *( STDMETHODCALLTYPE *GetD3DDevice )( 
            IMGDFSimHost * This);
        
        ID2D1Device *( STDMETHODCALLTYPE *GetD2DDevice )( 
            IMGDFSimHost * This);
        
        void ( STDMETHODCALLTYPE *GetDebug )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFDebug **debug);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCPUCounter )( 
            IMGDFSimHost * This,
            /* [in] */ const small *name,
            /* [out] */ IMGDFPerformanceCounter **counter);
        
        void ( STDMETHODCALLTYPE *GetSaves )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFSaveManager **save);
        
        void ( STDMETHODCALLTYPE *GetGame )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFGame **game);
        
        void ( STDMETHODCALLTYPE *GetSound )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFSoundManager **manager);
        
        void ( STDMETHODCALLTYPE *GetStatistics )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFStatisticsManager **statistics);
        
        void ( STDMETHODCALLTYPE *GetInput )( 
            IMGDFSimHost * This,
            /* [out] */ IMGDFInputManager **manager);
        
        void ( STDMETHODCALLTYPE *ShutDown )( 
            IMGDFSimHost * This);
        
        void ( STDMETHODCALLTYPE *QueueShutDown )( 
            IMGDFSimHost * This);
        
        END_INTERFACE
    } IMGDFSimHostVtbl;

    interface IMGDFSimHost
    {
        CONST_VTBL struct IMGDFSimHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFSimHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFSimHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFSimHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFSimHost_SetLoggingLevel(This,level)	\
    ( (This)->lpVtbl -> SetLoggingLevel(This,level) ) 

#define IMGDFSimHost_GetLoggingLevel(This)	\
    ( (This)->lpVtbl -> GetLoggingLevel(This) ) 

#define IMGDFSimHost_Log(This,sender,message,level)	\
    ( (This)->lpVtbl -> Log(This,sender,message,level) ) 


#define IMGDFSimHost_FatalError(This,sender,message)	\
    ( (This)->lpVtbl -> FatalError(This,sender,message) ) 

#define IMGDFSimHost_GetRenderSettings(This,settings)	\
    ( (This)->lpVtbl -> GetRenderSettings(This,settings) ) 

#define IMGDFSimHost_GetTimer(This,timer)	\
    ( (This)->lpVtbl -> GetTimer(This,timer) ) 

#define IMGDFSimHost_GetMGDFVersion(This)	\
    ( (This)->lpVtbl -> GetMGDFVersion(This) ) 

#define IMGDFSimHost_GetVFS(This,vfs)	\
    ( (This)->lpVtbl -> GetVFS(This,vfs) ) 

#define IMGDFSimHost_GetD3DDevice(This)	\
    ( (This)->lpVtbl -> GetD3DDevice(This) ) 

#define IMGDFSimHost_GetD2DDevice(This)	\
    ( (This)->lpVtbl -> GetD2DDevice(This) ) 

#define IMGDFSimHost_GetDebug(This,debug)	\
    ( (This)->lpVtbl -> GetDebug(This,debug) ) 

#define IMGDFSimHost_CreateCPUCounter(This,name,counter)	\
    ( (This)->lpVtbl -> CreateCPUCounter(This,name,counter) ) 


#define IMGDFSimHost_GetSaves(This,save)	\
    ( (This)->lpVtbl -> GetSaves(This,save) ) 

#define IMGDFSimHost_GetGame(This,game)	\
    ( (This)->lpVtbl -> GetGame(This,game) ) 

#define IMGDFSimHost_GetSound(This,manager)	\
    ( (This)->lpVtbl -> GetSound(This,manager) ) 

#define IMGDFSimHost_GetStatistics(This,statistics)	\
    ( (This)->lpVtbl -> GetStatistics(This,statistics) ) 

#define IMGDFSimHost_GetInput(This,manager)	\
    ( (This)->lpVtbl -> GetInput(This,manager) ) 

#define IMGDFSimHost_ShutDown(This)	\
    ( (This)->lpVtbl -> ShutDown(This) ) 

#define IMGDFSimHost_QueueShutDown(This)	\
    ( (This)->lpVtbl -> QueueShutDown(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFSimHost_INTERFACE_DEFINED__ */


#ifndef __IMGDFModule_INTERFACE_DEFINED__
#define __IMGDFModule_INTERFACE_DEFINED__

/* interface IMGDFModule */
/* [local][object][uuid] */ 


EXTERN_C const IID IID_IMGDFModule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08D1F4AD-366B-4659-B4A6-AA04B458D6DB")
    IMGDFModule : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE STNew( 
            IMGDFSimHost *host,
            const wchar_t *workingFolder) = 0;
        
        virtual BOOL STDMETHODCALLTYPE STUpdate( 
            IMGDFSimHost *host,
            double elapsedTime) = 0;
        
        virtual void STDMETHODCALLTYPE STShutDown( 
            IMGDFSimHost *host) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTBeforeFirstDraw( 
            IMGDFRenderHost *host) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTDraw( 
            IMGDFRenderHost *host,
            double alpha) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTBeforeBackBufferChange( 
            IMGDFRenderHost *host) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTBackBufferChange( 
            IMGDFRenderHost *host) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTBeforeDeviceReset( 
            IMGDFRenderHost *host) = 0;
        
        virtual BOOL STDMETHODCALLTYPE RTDeviceReset( 
            IMGDFRenderHost *host) = 0;
        
        virtual void STDMETHODCALLTYPE Panic( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IMGDFModuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMGDFModule * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMGDFModule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMGDFModule * This);
        
        BOOL ( STDMETHODCALLTYPE *STNew )( 
            IMGDFModule * This,
            IMGDFSimHost *host,
            const wchar_t *workingFolder);
        
        BOOL ( STDMETHODCALLTYPE *STUpdate )( 
            IMGDFModule * This,
            IMGDFSimHost *host,
            double elapsedTime);
        
        void ( STDMETHODCALLTYPE *STShutDown )( 
            IMGDFModule * This,
            IMGDFSimHost *host);
        
        BOOL ( STDMETHODCALLTYPE *RTBeforeFirstDraw )( 
            IMGDFModule * This,
            IMGDFRenderHost *host);
        
        BOOL ( STDMETHODCALLTYPE *RTDraw )( 
            IMGDFModule * This,
            IMGDFRenderHost *host,
            double alpha);
        
        BOOL ( STDMETHODCALLTYPE *RTBeforeBackBufferChange )( 
            IMGDFModule * This,
            IMGDFRenderHost *host);
        
        BOOL ( STDMETHODCALLTYPE *RTBackBufferChange )( 
            IMGDFModule * This,
            IMGDFRenderHost *host);
        
        BOOL ( STDMETHODCALLTYPE *RTBeforeDeviceReset )( 
            IMGDFModule * This,
            IMGDFRenderHost *host);
        
        BOOL ( STDMETHODCALLTYPE *RTDeviceReset )( 
            IMGDFModule * This,
            IMGDFRenderHost *host);
        
        void ( STDMETHODCALLTYPE *Panic )( 
            IMGDFModule * This);
        
        END_INTERFACE
    } IMGDFModuleVtbl;

    interface IMGDFModule
    {
        CONST_VTBL struct IMGDFModuleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMGDFModule_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMGDFModule_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMGDFModule_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMGDFModule_STNew(This,host,workingFolder)	\
    ( (This)->lpVtbl -> STNew(This,host,workingFolder) ) 

#define IMGDFModule_STUpdate(This,host,elapsedTime)	\
    ( (This)->lpVtbl -> STUpdate(This,host,elapsedTime) ) 

#define IMGDFModule_STShutDown(This,host)	\
    ( (This)->lpVtbl -> STShutDown(This,host) ) 

#define IMGDFModule_RTBeforeFirstDraw(This,host)	\
    ( (This)->lpVtbl -> RTBeforeFirstDraw(This,host) ) 

#define IMGDFModule_RTDraw(This,host,alpha)	\
    ( (This)->lpVtbl -> RTDraw(This,host,alpha) ) 

#define IMGDFModule_RTBeforeBackBufferChange(This,host)	\
    ( (This)->lpVtbl -> RTBeforeBackBufferChange(This,host) ) 

#define IMGDFModule_RTBackBufferChange(This,host)	\
    ( (This)->lpVtbl -> RTBackBufferChange(This,host) ) 

#define IMGDFModule_RTBeforeDeviceReset(This,host)	\
    ( (This)->lpVtbl -> RTBeforeDeviceReset(This,host) ) 

#define IMGDFModule_RTDeviceReset(This,host)	\
    ( (This)->lpVtbl -> RTDeviceReset(This,host) ) 

#define IMGDFModule_Panic(This)	\
    ( (This)->lpVtbl -> Panic(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMGDFModule_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


