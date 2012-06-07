/* 
** NetXMS - Network Management System
** Dummy SMS driver for debugging
** Copyright (C) 2012 Alex Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: main.cpp
**
**/

#include <nms_common.h>
#include <nxsrvapi.h>
#include <nxdbapi.h>
#include <nms_util.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *pszInitArgs)
{
	DbgPrintf(1, _T("Dummy SMS Driver loaded, set debug=6 or higher to see actual messages"));
	return true;
}

extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	DbgPrintf(6, _T("DummySMS: phone=\"%s\", text=\"%s\""), pszPhoneNumber, pszText);
}


extern "C" void EXPORT SMSDriverUnload()
{
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hInstance);
	}
	return TRUE;
}

#endif

