/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxcrashdump.h
** Shared definitions for crash dump IPC between a monitored process (crash
** handler in libnetxms) and the out-of-process dump generator (nxcrashsrv).
**
**/

#ifndef _nxcrashdump_h_
#define _nxcrashdump_h_

#include <nms_common.h>

#ifdef _WIN32

/**
 * Crash dump IPC protocol version. Bumped when the layout of CrashDumpRequest changes.
 */
#define NXCRASH_PROTOCOL_VERSION    1

/**
 * Time (in milliseconds) the crashing process waits for the dump to be written.
 */
#define NXCRASH_DUMP_TIMEOUT        30000

/**
 * Crash dump request placed in the shared memory section. The crashing process
 * fills faultingThreadId and exceptionPointers from within its exception filter;
 * the remaining fields are set when the handler is installed.
 */
struct CrashDumpRequest
{
   uint32_t protocolVersion;     // set by client, validated by server
   uint32_t faultingThreadId;    // ID of the thread that raised the exception
   uint64_t exceptionPointers;   // address of EXCEPTION_POINTERS in client address space
   uint32_t fullDump;            // non-zero to request full memory dump
   WCHAR processName[64];        // name of crashing process (e.g. "netxmsd")
};

/**
 * Build the name of an IPC object (shared memory section or event) from the
 * crashing process ID. Both client and server derive identical names this way.
 * Buffer must be at least 64 characters long.
 */
static inline void BuildCrashDumpObjectName(WCHAR *buffer, const WCHAR *suffix, uint32_t pid)
{
   _snwprintf(buffer, 64, L"Local\\NetXMS-Crash-%s-%u", suffix, pid);
}

#endif   /* _WIN32 */

#endif
