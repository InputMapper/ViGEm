/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifdef _MSC_VER
#pragma once
#endif

#include <initguid.h>
#include <public.h>

#ifdef VIGEM_EXPORTS
#define VIGEM_API __declspec(dllexport)
#else
#define VIGEM_API __declspec(dllimport)
#endif

#define VIGEM_TARGETS_MAX   4

typedef enum _VIGEM_ERRORS
{
    VIGEM_ERROR_NONE = 0x0000,
    VIGEM_ERROR_BUS_NOT_FOUND,
    VIGEM_ERROR_NO_FREE_SLOT,
    VIGEM_ERROR_INVALID_TARGET
} VIGEM_ERROR;

#define VIGEM_SUCCESS(_val_) (_val_ == VIGEM_ERROR_NONE)

//
// Represents a virtual gamepad object.
// 
typedef struct _VIGEM_TARGET
{
    IN ULONG Size;
    IN USHORT Version;
    IN ULONG SerialNo;
} VIGEM_TARGET, *PVIGEM_TARGET;

//
// Initializes a virtual gamepad object.
// 
VOID FORCEINLINE VIGEM_TARGET_INIT(
    _Out_ PVIGEM_TARGET Target
)
{
    RtlZeroMemory(Target, sizeof(VIGEM_TARGET));

    Target->Size = sizeof(VIGEM_TARGET);
    Target->Version = VIGEM_COMMON_VERSION;
}

typedef VOID(CALLBACK* VIGEM_XUSB_NOTIFICATION)(
    VIGEM_TARGET Target, 
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber);

typedef VOID(CALLBACK* VIGEM_DS4_NOTIFICATION)(
    VIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    DS4_LIGHTBAR_COLOR LightbarColor);

VIGEM_API VIGEM_ERROR vigem_init();

VIGEM_API VOID vigem_shutdown();

VIGEM_API VIGEM_ERROR vigem_register_xusb_notification(
    IN VIGEM_XUSB_NOTIFICATION Notification, 
    IN VIGEM_TARGET Target);

VIGEM_API VIGEM_ERROR vigem_register_ds4_notification(
    IN VIGEM_DS4_NOTIFICATION Notification,
    IN VIGEM_TARGET Target);

VIGEM_API VIGEM_ERROR vigem_target_plugin(
    _In_ VIGEM_TARGET_TYPE Type,
    _Out_ PVIGEM_TARGET Target);

VIGEM_API VIGEM_ERROR vigem_xusb_submit_report(
    _In_ VIGEM_TARGET Target,
    _In_ XUSB_REPORT Report);

VIGEM_API VIGEM_ERROR vigem_ds4_submit_report(
    _In_ VIGEM_TARGET Target,
    _In_ DS4_REPORT Report);

