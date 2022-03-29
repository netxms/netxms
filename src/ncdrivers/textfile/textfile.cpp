/*
** NetXMS - Network Management System
** Notification channel driver that writes messages to text file
** Copyright (C) 2019-2021 Raden Solutions
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
** File: textfile.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>

#define DEBUG_TAG _T("ncd.textfile")

static const NCConfigurationTemplate s_config(false, false); 

/**
 * Text driver class
 */
class TextFileDriver : public NCDriver
{
private:
   TCHAR m_fileName[MAX_PATH];

public:
   TextFileDriver(const TCHAR *fileName);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Create driver instance
 */
TextFileDriver::TextFileDriver(const TCHAR *fileName)
{
   _tcslcpy(m_fileName, fileName, MAX_PATH);
}

/**
 * Driver send method
 */
int TextFileDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;
   FILE *f = _tfopen(m_fileName, _T("a"));
   if (f == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot open file %s (%s)"), m_fileName, _tcserror(errno));
      return -1;
   }

   result = (_fputts(body, f) >= 0) ? 0 : -1;
   _fputts(_T("\n"), f);

   fclose(f);
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(TextFile, &s_config)
{
   const TCHAR *fileName = config->getValue(_T("/TextFile/OutputFile"));
   if (fileName == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create driver instance (output file name not set)"));
      return nullptr;
   }
   return new TextFileDriver(fileName);
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
