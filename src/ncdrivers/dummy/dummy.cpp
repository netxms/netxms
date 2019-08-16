/* 
** NetXMS - Network Management System
** Dummy SMS driver for debugging
** Copyright (C) 2012-2019 Raden Solutions
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

#include <ncdrv.h>
#include <nms_util.h>

#define DEBUG_TAG _T("ncd.dummy")

/**
 * Text driver class
 */
class DummyDriver : public NCDriver
{
private:
   TCHAR m_fileName[MAX_PATH];

public:
   DummyDriver(); 
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Init driver
 */
DummyDriver::DummyDriver()
{
	nxlog_debug_tag(DEBUG_TAG, 1, _T("Dummy SMS Driver loaded, set debug=6 or higher to see actual messages"));
}

/**
 * Send SMS
 */
bool DummyDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("recipient=\"%s\", subject=\"%s\", text=\"%s\""), recipient, subject, body);
   return true;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Dummy, NULL)
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
