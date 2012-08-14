/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#include "libnxlp.h"


//
// Expand file name
//   Can be used strftime placeholders and external commands enclosed in ``
//

const TCHAR *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize)
{
	time_t t;
	struct tm *ltm;
#if HAVE_LOCALTIME_R
	struct tm tmBuff;
#endif
	TCHAR temp[8192], command[1024];
	size_t outpos = 0;

	t = time(NULL);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t, &tmBuff);
#else
	ltm = localtime(&t);
#endif
	_tcsftime(temp, 8192, name, ltm);

	for(int i = 0; (temp[i] != 0) && (outpos < bufSize - 1); i++)
	{
		if (temp[i] == _T('`'))
		{
			int j = ++i;
			while((temp[j] != _T('`')) && (temp[j] != 0))
				j++;
			int len = min(j - i, 1023);
			memcpy(command, &temp[i], len * sizeof(TCHAR));
			command[len] = 0;

			FILE *p = _tpopen(command, _T("r"));
			if (p != NULL)
			{
				char result[1024];

				int rc = (int)fread(result, 1, 1023, p);
				pclose(p);

				if (rc > 0)
				{
					result[rc] = 0;
					char *lf = strchr(result, '\n');
					if (lf != NULL)
						*lf = 0;

					len = (int)min(strlen(result), bufSize - outpos - 1);
#ifdef UNICODE
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, result, len, &buffer[outpos], len);
#else
					memcpy(&buffer[outpos], result, len);
#endif
					outpos += len;
				}
			}

			i = j;
		}
		else
		{
			buffer[outpos++] = temp[i];
		}
	}

	buffer[outpos] = 0;
	return buffer;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */


//
// NetWare library entry point
//

#ifdef _NETWARE

int _init(void)
{
   return 0;
}

int _fini(void)
{
   return 0;
}

#endif
