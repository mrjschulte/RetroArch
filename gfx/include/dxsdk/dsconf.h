/*==========================================================================;
 *
 *  Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *  File:       dsconf.h
 *  Content:    DirectSound configuration interface include file
 *
 **************************************************************************/

#ifndef __DSCONF_INCLUDED__
#define __DSCONF_INCLUDED__
/*#include <winapifamily.h>*/

/*#pragma region Desktop Family*/
/*#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)*/


#ifndef __DSOUND_INCLUDED__
#error dsound.h not included
#endif // __DSOUND_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// DirectSound Configuration Component GUID {11AB3EC0-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(CLSID_DirectSoundPrivate, 0x11ab3ec0, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);


//
// DirectSound Device Properties {84624F82-25EC-11d1-A4D8-00C04FC28ACA}
//

DEFINE_GUID(DSPROPSETID_DirectSoundDevice, 0x84624f82, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);

typedef enum
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A = 1,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1 = 2,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1 = 3,
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W = 4,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A = 5,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W = 6,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A = 7,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W = 8,
} DSPROPERTY_DIRECTSOUNDDEVICE;

#if DIRECTSOUND_VERSION >= 0x0700
#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A
#endif // UNICODE
#else // DIRECTSOUND_VERSION >= 0x0700
#define DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1
#endif // DIRECTSOUND_VERSION >= 0x0700

typedef enum
{
    DIRECTSOUNDDEVICE_TYPE_EMULATED,
    DIRECTSOUNDDEVICE_TYPE_VXD,
    DIRECTSOUNDDEVICE_TYPE_WDM
} DIRECTSOUNDDEVICE_TYPE;

typedef enum
{
    DIRECTSOUNDDEVICE_DATAFLOW_RENDER,
    DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE
} DIRECTSOUNDDEVICE_DATAFLOW;


typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA
{
    LPSTR                       DeviceName; // waveIn/waveOut device name
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;   // Data flow (i.e. waveIn or waveOut)
    GUID                        DeviceId;   // DirectSound device id
} DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA
{
    LPWSTR                      DeviceName; // waveIn/waveOut device name
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;   // Data flow (i.e. waveIn or waveOut)
    GUID                        DeviceId;   // DirectSound device id
} DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA;

#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA
#endif // UNICODE

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
{
    GUID                        DeviceId;               // DirectSound device id
    CHAR                        DescriptionA[0x100];    // Device description (ANSI)
    WCHAR                       DescriptionW[0x100];    // Device description (Unicode)
    CHAR                        ModuleA[MAX_PATH];      // Device driver module (ANSI)
    WCHAR                       ModuleW[MAX_PATH];      // Device driver module (Unicode)
    DIRECTSOUNDDEVICE_TYPE      Type;                   // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;               // Device dataflow
    ULONG                       WaveDeviceId;           // Wave device id
    ULONG                       Devnode;                // Devnode (or DevInst)
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
{
    DIRECTSOUNDDEVICE_TYPE      Type;           // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;       // Device dataflow
    GUID                        DeviceId;       // DirectSound device id
    LPSTR                       Description;    // Device description
    LPSTR                       Module;         // Device driver module
    LPSTR                       Interface;      // Device interface
    ULONG                       WaveDeviceId;   // Wave device id
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
{
    DIRECTSOUNDDEVICE_TYPE      Type;           // Device type
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;       // Device dataflow
    GUID                        DeviceId;       // DirectSound device id
    LPWSTR                      Description;    // Device description
    LPWSTR                      Module;         // Device driver module
    LPWSTR                      Interface;      // Device interface
    ULONG                       WaveDeviceId;   // Wave device id
} DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA;

#if DIRECTSOUND_VERSION >= 0x0700
#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA
#endif // UNICODE
#else // DIRECTSOUND_VERSION >= 0x0700
#define DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA
#endif // DIRECTSOUND_VERSION >= 0x0700

typedef BOOL (CALLBACK *LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK1)(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA, LPVOID);
typedef BOOL (CALLBACK *LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKA)(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA, LPVOID);
typedef BOOL (CALLBACK *LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKW)(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA, LPVOID);

#if DIRECTSOUND_VERSION >= 0x0700
#ifdef UNICODE
#define LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKW
#else // UNICODE
#define LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKA
#endif // UNICODE
#else // DIRECTSOUND_VERSION >= 0x0700
#define LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK1
#endif // DIRECTSOUND_VERSION >= 0x0700

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA
{
    LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK1 Callback;   // Callback function pointer
    LPVOID                                  Context;    // Callback function context argument
} DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA
{
    LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKA Callback;   // Callback function pointer
    LPVOID                                  Context;    // Callback function context argument
} DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA
{
    LPFNDIRECTSOUNDDEVICEENUMERATECALLBACKW Callback;   // Callback function pointer
    LPVOID                                  Context;    // Callback function context argument
} DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA;

#if DIRECTSOUND_VERSION >= 0x0700
#ifdef UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA
#else // UNICODE
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA
#endif // UNICODE
#else // DIRECTSOUND_VERSION >= 0x0700
#define DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA
#define PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA
#endif // DIRECTSOUND_VERSION >= 0x0700


#ifdef __cplusplus
}
#endif // __cplusplus


/*#endif*/ /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
/*#pragma endregion*/

#endif  // __DSCONF_INCLUDED__

