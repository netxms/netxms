/* $Id$ */

/* 
** NetXMS Linux subagent
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: hddinfo.cpp
**
**/

#include <linux_subagent.h>
#include <ata.h>
#include <sys/ioctl.h>


//
// Constants
//

#define HDIO_DRIVE_CMD            0x031f
#define HDIO_DRIVE_TASK           0x031e
#define HDIO_DRIVE_TASKFILE       0x031d
#define HDIO_GET_IDENTITY         0x030d

#define HDIO_DRIVE_CMD_OFFSET     4

#define ATTR_TYPE_BYTE        0
#define ATTR_TYPE_HEX_STRING  1


//
// Get value of specific attribute from SMART_ATA_VALUES structure
//

static BOOL GetAttributeValue(ATA_SMART_VALUES *pSmartValues, BYTE bAttr,
		TCHAR *pValue, int nType)
{
	int i;
	BOOL bResult = FALSE;

	for(i = 0; i < NUMBER_ATA_SMART_ATTRIBUTES; i++)
		if (pSmartValues->vendor_attributes[i].id == bAttr)
		{
			switch(nType)
			{
				case ATTR_TYPE_BYTE:
					ret_uint(pValue, pSmartValues->vendor_attributes[i].raw[0]);
					break;
				case ATTR_TYPE_HEX_STRING:
					BinToStr(pSmartValues->vendor_attributes[i].raw, 6, pValue);
					break;
			}
			bResult = TRUE;
			break;
		}
	return bResult;
}


//
// Handler for PhysicalDisk.*
//

LONG H_PhysicalDiskInfo(const TCHAR *pszParam, const TCHAR *pszArg, TCHAR *pValue, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_ERROR, nDisk, nCmd;
	char szBuffer[MAX_PATH];
	int hDevice;
	BYTE ioBuff[1024];

	if (!AgentGetParameterArgA(pszParam, 1, szBuffer, MAX_PATH))
		return SYSINFO_RC_UNSUPPORTED;

	// Open device
	hDevice = open(szBuffer, O_RDWR);
	if (hDevice != -1)
	{
		// Setup request's common fields
		memset(ioBuff, 0, sizeof(ioBuff));
		ioBuff[0] = ATA_SMART_CMD;

		// Setup request-dependent fields
		switch(pszArg[0])
		{
			case 'A':   // Generic SMART attribute
			case 'T':   // Temperature
				ioBuff[1] = 1;
				ioBuff[2] = ATA_SMART_READ_VALUES;
				ioBuff[3] = 1;
				nCmd = HDIO_DRIVE_CMD;
				break;
			case 'S':   // Disk status reported by SMART
				ioBuff[1] = ATA_SMART_STATUS;
				ioBuff[4] = SMART_CYL_HI;
				ioBuff[5] = SMART_CYL_LOW;
				nCmd = HDIO_DRIVE_TASK;
				break;
			default:
				nRet = SYSINFO_RC_UNSUPPORTED;
				break;
		}


		if (ioctl(hDevice, nCmd, ioBuff) >= 0)
		{
			switch(pszArg[0])
			{
				case 'A':   // Generic attribute
					if (AgentGetParameterArgA(pszParam, 2, szBuffer, 128))
					{
						LONG nAttr;
						char *eptr;

						nAttr = strtol(szBuffer, &eptr, 0);
						if ((*eptr != 0) || (nAttr <= 0) || (nAttr > 255))
						{
							nRet = SYSINFO_RC_UNSUPPORTED;
						}
						else
						{
							if (GetAttributeValue((ATA_SMART_VALUES *)&ioBuff[HDIO_DRIVE_CMD_OFFSET],
										(BYTE)nAttr, pValue, ATTR_TYPE_HEX_STRING))
								nRet = SYSINFO_RC_SUCCESS;
						}
					}
					else
					{
						nRet = SYSINFO_RC_UNSUPPORTED;
					}
					break;
				case 'T':   // Temperature
					if (GetAttributeValue((ATA_SMART_VALUES *)&ioBuff[HDIO_DRIVE_CMD_OFFSET],
								0xC2, pValue, ATTR_TYPE_BYTE))
					{
						nRet = SYSINFO_RC_SUCCESS;
					}
					break;
				case 'S':   // SMART status
					if ((ioBuff[4] == SMART_CYL_HI) && (ioBuff[5] == SMART_CYL_LOW))
					{
						ret_int(pValue, 0);  // Status is OK
					}
					else if ((ioBuff[4] == 0x2C) && (ioBuff[5] == 0xF4))
					{
						ret_int(pValue, 1);  // Status is BAD
					}
					else
					{
						ret_int(pValue, 2);  // Status is UNKNOWN
					}
					nRet = SYSINFO_RC_SUCCESS;
					break;
				default:
					nRet = SYSINFO_RC_UNSUPPORTED;
					break;
			}
		}

		close(hDevice);
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
