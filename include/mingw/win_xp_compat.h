/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: mingw/win_xp_compat.h
**
** Windows XP compatibility shims for the mingw XP agent build
** (config.mingw.xp, built at _WIN32_WINNT=0x0501). Provides the Vista+ APIs
** that the NetXMS core threading layer relies on but that do not exist on
** Windows XP:
**   - GetTickCount64
**   - inet_pton / inet_ntop
**   - CONDITION_VARIABLE and Initialize/Sleep/Wake/WakeAll/Delete
**   - swprintf_s (secure wide printf used by toolchain COM headers)
**
** Active only for Windows targets built below Vista. On Vista+ the real APIs
** are used and this header is inert, so non-XP builds are unaffected.
**
**/

#ifndef _mingw_win_xp_compat_h_
#define _mingw_win_xp_compat_h_

#if defined(_WIN32) && (_WIN32_WINNT < 0x0600)

#ifdef __cplusplus
extern "C" {
#endif

// Newer SDK constants the legacy mingw32-xp headers predate. Guarded so they
// are only defined if the toolchain headers do not already provide them.
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64          12
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_ARM64
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64  14
#endif
// WTS_ANY_SESSION (wtsapi32.h) is Vista+.
#ifndef WTS_ANY_SESSION
#define WTS_ANY_SESSION  ((DWORD)-2)
#endif

/**
 * GetTickCount64 - Windows XP has only the 32-bit GetTickCount (which wraps
 * every ~49.7 days). Derive a wrap-free 64-bit millisecond clock from the
 * high-resolution performance counter, which is available on XP. Callers use
 * this only for elapsed-time deltas, so an arbitrary epoch is fine.
 */
static inline ULONGLONG GetTickCount64(void)
{
   LARGE_INTEGER freq, counter;
   if (!QueryPerformanceFrequency(&freq) || (freq.QuadPart == 0))
      return (ULONGLONG)GetTickCount();
   QueryPerformanceCounter(&counter);
   // Compute milliseconds without overflowing: whole seconds * 1000 + remainder.
   return (ULONGLONG)(counter.QuadPart / freq.QuadPart) * 1000 +
          (ULONGLONG)((counter.QuadPart % freq.QuadPart) * 1000 / freq.QuadPart);
}

/**
 * inet_pton / inet_ntop - added to Winsock in Vista. Implement them on XP via
 * WSAStringToAddress / WSAAddressToString, which are available on XP. Winsock
 * must be initialized (WSAStartup) before use, which NetXMS does at startup.
 */
static inline int inet_pton(int af, const char *src, void *dst)
{
   struct sockaddr_storage ss;
   int addressLength = (int)sizeof(ss);
   char buffer[64];

   memset(&ss, 0, sizeof(ss));
   lstrcpynA(buffer, src, (int)sizeof(buffer));
   if (WSAStringToAddressA(buffer, af, NULL, (struct sockaddr *)&ss, &addressLength) != 0)
      return 0;   // invalid address string

   if (af == AF_INET)
   {
      memcpy(dst, &((struct sockaddr_in *)&ss)->sin_addr, sizeof(struct in_addr));
      return 1;
   }
   if (af == AF_INET6)
   {
      memcpy(dst, &((struct sockaddr_in6 *)&ss)->sin6_addr, sizeof(struct in6_addr));
      return 1;
   }
   return -1;   // unsupported address family
}

static inline const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
   struct sockaddr_storage ss;
   DWORD stringLength = (DWORD)size;

   memset(&ss, 0, sizeof(ss));
   ss.ss_family = (short)af;
   if (af == AF_INET)
      memcpy(&((struct sockaddr_in *)&ss)->sin_addr, src, sizeof(struct in_addr));
   else if (af == AF_INET6)
      memcpy(&((struct sockaddr_in6 *)&ss)->sin6_addr, src, sizeof(struct in6_addr));
   else
      return NULL;

   if (WSAAddressToStringA((struct sockaddr *)&ss, (DWORD)sizeof(ss), NULL, dst, &stringLength) != 0)
      return NULL;
   return dst;
}

/**
 * swprintf_s - the security-enhanced wide printf was added to both msvcrt.dll and
 * ntdll.dll only in Windows Vista. Toolchain headers such as <comutil.h> (pulled in
 * by <comdef.h> for COM support, used by the winnt subagent and libnxlp) call it,
 * which creates an unresolved Vista+ import that prevents the module from loading on
 * XP. Provide a bounded, always-null-terminating implementation over the classic
 * _vsnwprintf (present on XP) and redirect calls to it. This matches the swprintf_s
 * contract closely enough for the toolchain's internal (integer/string) formatting.
 */
static inline int __cdecl __nx_swprintf_s(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
   if ((buffer == NULL) || (size == 0))
      return -1;
   va_list args;
   va_start(args, format);
   int rc = _vsnwprintf(buffer, size, format, args);
   va_end(args);
   if ((rc < 0) || ((size_t)rc >= size))
      buffer[size - 1] = 0;   // guarantee null termination on overflow/truncation
   return rc;
}
#define swprintf_s __nx_swprintf_s

/**
 * CONDITION_VARIABLE emulation.
 *
 * The Vista condition variable is emulated with a counting semaphore plus a
 * waiter count. This is correct for the way NetXMS uses condition variables
 * (Condition in nms_threads.h and win_rwlock_t in winrwlock.h), which all
 * satisfy three invariants:
 *   1. Sleep and Wake are always called while the associated CRITICAL_SECTION
 *      is held, so the waiter count is serialized by that external lock and
 *      needs no separate internal lock.
 *   2. Callers loop on an authoritative predicate protected by that same lock,
 *      so spurious wakeups are harmless.
 *   3. Every predicate-changing transition is paired with a Wake, so a blocked
 *      waiter is always re-signalled when the state it waits for becomes true.
 * A counting semaphore preserves a signal raised in the window between
 * releasing the lock and blocking, so no wakeup is ever lost; the timeout path
 * drains a stray count to keep the semaphore from drifting positive.
 */
// The CONDITION_VARIABLE type itself exists in the mingw headers even below
// Vista (RTL_CONDITION_VARIABLE, a single opaque PVOID 'Ptr'); only the
// functions are hidden. So do not redefine the type - reuse it and stash the
// emulation state (a counting semaphore + waiter count) in the Ptr slot.
typedef struct _NX_CONDITION_VARIABLE_XP
{
   HANDLE semaphore;
   volatile LONG waiters;
} NX_CONDITION_VARIABLE_XP;

static inline void InitializeConditionVariable(PCONDITION_VARIABLE cv)
{
   NX_CONDITION_VARIABLE_XP *state =
      (NX_CONDITION_VARIABLE_XP *)HeapAlloc(GetProcessHeap(), 0, sizeof(NX_CONDITION_VARIABLE_XP));
   state->semaphore = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
   state->waiters = 0;
   cv->Ptr = state;
}

/**
 * Release the resources owned by an emulated condition variable. The real
 * Vista API has no counterpart (condition variables need no cleanup there), so
 * call sites guard this with _WIN32_WINNT < 0x0600.
 */
static inline void DeleteConditionVariable(PCONDITION_VARIABLE cv)
{
   NX_CONDITION_VARIABLE_XP *state = (NX_CONDITION_VARIABLE_XP *)cv->Ptr;
   if (state != NULL)
   {
      if (state->semaphore != NULL)
         CloseHandle(state->semaphore);
      HeapFree(GetProcessHeap(), 0, state);
      cv->Ptr = NULL;
   }
}

static inline BOOL SleepConditionVariableCS(PCONDITION_VARIABLE cv, PCRITICAL_SECTION externalLock, DWORD milliseconds)
{
   NX_CONDITION_VARIABLE_XP *state = (NX_CONDITION_VARIABLE_XP *)cv->Ptr;
   state->waiters++;   // serialized by the caller-held externalLock
   LeaveCriticalSection(externalLock);
   DWORD rc = WaitForSingleObject(state->semaphore, milliseconds);
   EnterCriticalSection(externalLock);
   state->waiters--;
   if (rc != WAIT_OBJECT_0)
   {
      // Timed out, but a Wake may have raced in and released a count for us.
      // Consume any such stray count so the semaphore does not drift positive
      // (drift would only cause extra harmless spurious wakeups, never a hang).
      if (WaitForSingleObject(state->semaphore, 0) == WAIT_OBJECT_0)
         rc = WAIT_OBJECT_0;
   }
   return (rc == WAIT_OBJECT_0) ? TRUE : FALSE;
}

static inline void WakeConditionVariable(PCONDITION_VARIABLE cv)
{
   NX_CONDITION_VARIABLE_XP *state = (NX_CONDITION_VARIABLE_XP *)cv->Ptr;
   if (state->waiters > 0)   // serialized by the caller-held externalLock
      ReleaseSemaphore(state->semaphore, 1, NULL);
}

static inline void WakeAllConditionVariable(PCONDITION_VARIABLE cv)
{
   NX_CONDITION_VARIABLE_XP *state = (NX_CONDITION_VARIABLE_XP *)cv->Ptr;
   LONG count = state->waiters;   // serialized by the caller-held externalLock
   if (count > 0)
      ReleaseSemaphore(state->semaphore, count, NULL);
}

#ifdef __cplusplus
}
#endif

#endif   /* _WIN32 && _WIN32_WINNT < 0x0600 */

#endif   /* _mingw_win_xp_compat_h_ */
