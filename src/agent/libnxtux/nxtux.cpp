/*
** NetXMS Tuxedo helper library
** Copyright (C) 2014-2021 Raden Solutions
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
** File: nxtux.cpp
**
**/

#include <nxtux.h>

/**
 * Connect count
 */
static int s_connectCount = 0;
static Mutex s_connectLock;

/**
 * Connect to Tuxedo app
 */
bool LIBNXTUX_EXPORTABLE TuxedoConnect()
{
   bool success = true;
   s_connectLock.lock();
   if (s_connectCount == 0)
   {
      if (tpinit(nullptr) != -1)
      {
         s_connectCount++;
      }
      else
      {
         nxlog_debug(3, _T("tpinit() call failed (%hs)"), tpstrerrordetail(tperrno, 0));
         success = false;
      }
   }
   else
   {
      s_connectCount++;
   }
   s_connectLock.unlock();
   return success;
}

/**
 * Disconnect from Tuxedo app
 */
void LIBNXTUX_EXPORTABLE TuxedoDisconnect()
{
   s_connectLock.lock();
   if (s_connectCount > 0)
   {
      s_connectCount--;
      if (s_connectCount == 0)
         tpterm();
   }
   s_connectLock.unlock();
}

/**
 * Helper function to get string field
 */
bool LIBNXTUX_EXPORTABLE CFgetString(FBFR32 *fb, FLDID32 fieldid, FLDOCC32 oc, char *buf, size_t size)
{
   FLDLEN32 len = (FLDLEN32)size;
   if (CFget32(fb, fieldid, oc, buf, &len, FLD_STRING) == -1)
   {
      buf[0] = 0;
      return false;
   }
   return true;
}

/**
 * Helper function for extracting executable name
 */
bool LIBNXTUX_EXPORTABLE CFgetExecutableName(FBFR32 *fb, FLDID32 fieldid, FLDOCC32 oc, char *buf, size_t size)
{
   char temp[MAX_PATH];
   if (!CFgetString(fb, fieldid, oc, temp, sizeof(temp)))
      return false;
   
   char *p = strrchr(temp, '/');
   if (p != nullptr)
      p++;
   else
      p = temp;
   strlcpy(buf, p, size);
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
