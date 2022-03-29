/* 
** NetXMS - Network Management System
** Dummy notification channel driver for debugging
** Copyright (C) 2012-2021 Raden Solutions
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
** File: dummy.cpp
**
**/

#include <ncdrv.h>

#define DEBUG_TAG _T("ncd.dummy")

static const NCConfigurationTemplate s_config(true, true);

/**
 * Dummy driver class
 */
class DummyDriver : public NCDriver
{
public:
   DummyDriver();
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Init driver
 */
DummyDriver::DummyDriver()
{
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Dummy notification channel driver instantiated, set debug level to 6 or higher to see actual messages"));
}

/**
 * Send SMS
 */
int DummyDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("recipient=\"%s\", subject=\"%s\", text=\"%s\""), recipient, subject, body);
   return 0;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Dummy, &s_config)
{
   return new DummyDriver();
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
