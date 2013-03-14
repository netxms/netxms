/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: msi.cpp
**
**/

#include "winnt_subagent.h"
#include <msi.h>

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value)
{
	value->addColumn(_T("NAME"));
	value->addColumn(_T("VERSION"));
	value->addColumn(_T("VENDOR"));
	value->addColumn(_T("DATE"));
	value->addColumn(_T("URL"));
	value->addColumn(_T("DESCRIPTION"));

	DWORD index = 0;
	TCHAR productId[40];
	while(MsiEnumProducts(index++, productId) == ERROR_SUCCESS)
	{
		TCHAR buffer[256];
		DWORD size = 256;
		if (MsiGetProductInfo(productId, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buffer, &size) == ERROR_SUCCESS)
		{
			value->addRow();
			value->set(0, buffer);

			size = 256;
			if (MsiGetProductInfo(productId, INSTALLPROPERTY_VERSIONSTRING, buffer, &size) == ERROR_SUCCESS)
			{
				value->set(1, buffer);
			}

			size = 256;
			if (MsiGetProductInfo(productId, INSTALLPROPERTY_PUBLISHER, buffer, &size) == ERROR_SUCCESS)
			{
				value->set(2, buffer);
			}

			size = 256;
			if (MsiGetProductInfo(productId, INSTALLPROPERTY_INSTALLDATE, buffer, &size) == ERROR_SUCCESS)
			{
				if (_tcslen(buffer) == 8)
				{
					// convert YYYYMMDD to UNIX timestamp
					struct tm t;
					memset(&t, 0, sizeof(struct tm));
					t.tm_mday = _tcstol(&buffer[6], NULL, 10);
					buffer[6] = 0;
					t.tm_mon = _tcstol(&buffer[4], NULL, 10) - 1;
					buffer[4] = 0;
					t.tm_year = _tcstol(buffer, NULL, 10) - 1900;
					t.tm_isdst = -1;
					t.tm_hour = 12;
					_sntprintf(buffer, 256, _T("%ld"), (long)mktime(&t));
					value->set(3, buffer);
				}
			}

			size = 256;
			if (MsiGetProductInfo(productId, INSTALLPROPERTY_URLINFOABOUT, buffer, &size) == ERROR_SUCCESS)
			{
				value->set(4, buffer);
			}
		}
	}

	return SYSINFO_RC_SUCCESS;
}
