/*
** NetXMS - Network Management System
** MinGW compatibility shim
**
** File: mingw/win_missing_defs.h
**
** Provides Windows API definitions that are present in the Microsoft Windows SDK
** but missing from (or incomplete in) the llvm-mingw / mingw-w64 headers. Include
** this header from Windows sources that need these definitions when building with
** MinGW. It is inert on MSVC, where the real SDK headers already provide them.
**
** Reference: Windows 7 SDK (winternl.h, WinSvc.h, wuapi.h).
**/

#ifndef _mingw_win_missing_defs_h_
#define _mingw_win_missing_defs_h_

#if defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>
#include <winternl.h>
#include <winsvc.h>
#include <wuapi.h>

/////////////////////////////////////////////////////////////////////////////
// Full SYSTEM_PROCESS_INFORMATION layout.
//
// mingw's <winternl.h> declares a trimmed _SYSTEM_PROCESS_INFORMATION that hides
// most fields behind reserved blobs, so members such as VirtualSize/WorkingSetSize
// are not accessible by name. This is the complete documented layout (Vista/7+)
// returned by NtQuerySystemInformation(SystemProcessInformation, ...). Consumers
// cast the query buffer to NX_SYSTEM_PROCESS_INFORMATION.
/////////////////////////////////////////////////////////////////////////////

typedef struct _NX_SYSTEM_PROCESS_INFORMATION
{
   ULONG NextEntryOffset;
   ULONG NumberOfThreads;
   LARGE_INTEGER WorkingSetPrivateSize;
   ULONG HardFaultCount;
   ULONG NumberOfThreadsHighWatermark;
   ULONGLONG CycleTime;
   LARGE_INTEGER CreateTime;
   LARGE_INTEGER UserTime;
   LARGE_INTEGER KernelTime;
   UNICODE_STRING ImageName;
   LONG BasePriority;
   HANDLE UniqueProcessId;
   HANDLE InheritedFromUniqueProcessId;
   ULONG HandleCount;
   ULONG SessionId;
   ULONG_PTR UniqueProcessKey;
   SIZE_T PeakVirtualSize;
   SIZE_T VirtualSize;
   ULONG PageFaultCount;
   SIZE_T PeakWorkingSetSize;
   SIZE_T WorkingSetSize;
   SIZE_T QuotaPeakPagedPoolUsage;
   SIZE_T QuotaPagedPoolUsage;
   SIZE_T QuotaPeakNonPagedPoolUsage;
   SIZE_T QuotaNonPagedPoolUsage;
   SIZE_T PagefileUsage;
   SIZE_T PeakPagefileUsage;
   SIZE_T PrivatePageCount;
   LARGE_INTEGER ReadOperationCount;
   LARGE_INTEGER WriteOperationCount;
   LARGE_INTEGER OtherOperationCount;
   LARGE_INTEGER ReadTransferCount;
   LARGE_INTEGER WriteTransferCount;
   LARGE_INTEGER OtherTransferCount;
} NX_SYSTEM_PROCESS_INFORMATION, *PNX_SYSTEM_PROCESS_INFORMATION;

/////////////////////////////////////////////////////////////////////////////
// Service trigger structures (winsvc.h in the Windows SDK; absent from mingw).
/////////////////////////////////////////////////////////////////////////////

#ifndef SERVICE_TRIGGER_INFO
typedef struct _SERVICE_TRIGGER_SPECIFIC_DATA_ITEM
{
   DWORD dwDataType;
   DWORD cbData;
   PBYTE pData;
} SERVICE_TRIGGER_SPECIFIC_DATA_ITEM, *PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM;

typedef struct _SERVICE_TRIGGER
{
   DWORD dwTriggerType;
   DWORD dwAction;
   GUID *pTriggerSubtype;
   DWORD cDataItems;
   PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM pDataItems;
} SERVICE_TRIGGER, *PSERVICE_TRIGGER;

typedef struct _SERVICE_TRIGGER_INFO
{
   DWORD cTriggers;
   PSERVICE_TRIGGER pTriggers;
   PBYTE pReserved;
} SERVICE_TRIGGER_INFO, *PSERVICE_TRIGGER_INFO;
#endif   /* SERVICE_TRIGGER_INFO */

/////////////////////////////////////////////////////////////////////////////
// Windows Update Agent interfaces (wuapi.h in the Windows SDK). mingw ships
// IAutomaticUpdates but not IAutomaticUpdates2 / IAutomaticUpdatesResults.
// IAutomaticUpdates2 adds a single method (get_Results) over IAutomaticUpdates,
// whose method set matches the SDK, so we derive from mingw's IAutomaticUpdates.
/////////////////////////////////////////////////////////////////////////////

#ifndef __IAutomaticUpdatesResults_INTERFACE_DEFINED__
#define __IAutomaticUpdatesResults_INTERFACE_DEFINED__
MIDL_INTERFACE("E7A4D634-7942-4DD9-A111-82228BA33901")
IAutomaticUpdatesResults : public IDispatch
{
public:
   virtual HRESULT STDMETHODCALLTYPE get_LastSearchSuccessDate(VARIANT *retval) = 0;
   virtual HRESULT STDMETHODCALLTYPE get_LastInstallationSuccessDate(VARIANT *retval) = 0;
};
__CRT_UUID_DECL(IAutomaticUpdatesResults, 0xE7A4D634, 0x7942, 0x4DD9, 0xA1, 0x11, 0x82, 0x22, 0x8B, 0xA3, 0x39, 0x01)
#endif   /* __IAutomaticUpdatesResults_INTERFACE_DEFINED__ */

#ifndef __IAutomaticUpdates2_INTERFACE_DEFINED__
#define __IAutomaticUpdates2_INTERFACE_DEFINED__
MIDL_INTERFACE("4A2F5C31-CFD9-410E-B7FB-29A653973A0F")
IAutomaticUpdates2 : public IAutomaticUpdates
{
public:
   virtual HRESULT STDMETHODCALLTYPE get_Results(IAutomaticUpdatesResults **retval) = 0;
};
__CRT_UUID_DECL(IAutomaticUpdates2, 0x4A2F5C31, 0xCFD9, 0x410E, 0xB7, 0xFB, 0x29, 0xA6, 0x53, 0x97, 0x3A, 0x0F)
#endif   /* __IAutomaticUpdates2_INTERFACE_DEFINED__ */

// CLSID_AutomaticUpdates is declared by mingw's <wuapi.h>, but no mingw import
// library provides its definition. Define it here; DECLSPEC_SELECTANY places it
// in a COMDAT section so multiple translation units merge without conflict.
EXTERN_C const DECLSPEC_SELECTANY CLSID CLSID_AutomaticUpdates =
   { 0xBFE18E9C, 0x6D87, 0x4450, { 0xB3, 0x7C, 0xE0, 0x2F, 0x0B, 0x37, 0x38, 0x03 } };

#endif   /* __MINGW32__ */

#endif   /* _mingw_win_missing_defs_h_ */
