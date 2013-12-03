/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: hddinfo.cpp
**
**/

#include "nxagentd.h"
#include <winioctl.h>
#include <ata.h>


//
// Constants
//

#define SMART_BUFFER_SIZE     512

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

LONG H_PhysicalDiskInfo(const TCHAR *pszParam, const TCHAR *pszArg, TCHAR *pValue)
{
   LONG nRet = SYSINFO_RC_ERROR, nDisk, nCmd;
   TCHAR szBuffer[128], *eptr;                     //
   BYTE pbValue[40];                               // 
   HANDLE hDevice;
   SENDCMDINPARAMS rq;
   SENDCMDOUTPARAMS *pResult;
   DWORD dwBytes;
   BOOL bSwapWords = FALSE;
   //memset(pbValue, 0, sizeof(BYTE) * 40);
   if (!AgentGetParameterArg(pszParam, 1, szBuffer, 128))
      return SYSINFO_RC_UNSUPPORTED;

   // Get physical disk number (zero-based)
   nDisk = _tcstol(szBuffer, &eptr, 0);             //
   if ((*eptr != 0) || (nDisk < 0) || (nDisk > 255))
      return SYSINFO_RC_UNSUPPORTED;

   // Open device
   _sntprintf(szBuffer, 128, _T("\\\\.\\PHYSICALDRIVE%d"), nDisk);      //
   hDevice = CreateFile(szBuffer, GENERIC_READ | GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if (hDevice != INVALID_HANDLE_VALUE)
   {
      // Setup request's common fields
      memset(&rq, 0, sizeof(SENDCMDINPARAMS));
      rq.bDriveNumber = (BYTE)nDisk;
      rq.irDriveRegs.bCommandReg = ATA_SMART_CMD;
	   rq.irDriveRegs.bCylHighReg = SMART_CYL_HI;
      rq.irDriveRegs.bCylLowReg = SMART_CYL_LOW;

      // Setup request-dependent fields
      switch(pszArg[0])
      {
         case _T('A'):   // Generic SMART attribute
         case _T('T'):   // Temperature
            rq.irDriveRegs.bFeaturesReg = ATA_SMART_READ_VALUES;
            rq.irDriveRegs.bSectorNumberReg = 1;
            rq.irDriveRegs.bSectorCountReg = 1;
            nCmd = SMART_RCV_DRIVE_DATA;
            break;
         case _T('S'):   // Disk status reported by SMART
            rq.irDriveRegs.bFeaturesReg = ATA_SMART_STATUS;
            nCmd = SMART_SEND_DRIVE_COMMAND;
            break;
         case _T('M'):   // Model
         case _T('N'):   // Serial number
         case _T('F'):   // Firmware
            rq.irDriveRegs.bCommandReg = ATA_IDENTIFY_DEVICE;
	         rq.irDriveRegs.bCylHighReg = 0;
            rq.irDriveRegs.bCylLowReg = 0;
            rq.irDriveRegs.bSectorCountReg = 1;
            nCmd = SMART_RCV_DRIVE_DATA;
            bSwapWords = TRUE;
            break;
         default:
            nRet = SYSINFO_RC_UNSUPPORTED;
            break;
      }

      // Allocate buffer for result
      pResult = (SENDCMDOUTPARAMS *)malloc(sizeof(SENDCMDOUTPARAMS) - 1 + SMART_BUFFER_SIZE);

      if (DeviceIoControl(hDevice, nCmd, &rq, sizeof(SENDCMDINPARAMS) - 1,
                          pResult, sizeof(SENDCMDOUTPARAMS) - 1 + SMART_BUFFER_SIZE,
                          &dwBytes, NULL))
      {
#if !(WORDS_BIGENDIAN)
         if (bSwapWords)
         {
            for(DWORD i = 0; i < dwBytes - sizeof(SENDCMDOUTPARAMS) + 1; i += 2)
               *((WORD *)&pResult->bBuffer[i]) = ntohs(*((WORD *)&pResult->bBuffer[i]));
         }
#endif
         switch(pszArg[0])
         {
            case _T('A'):   // Generic attribute
               if (AgentGetParameterArg(pszParam, 2, szBuffer, 128))
               {
                  LONG nAttr;

                  nAttr = _tcstol(szBuffer, &eptr, 0);
                  if ((*eptr != 0) || (nAttr <= 0) || (nAttr > 255))
                  {
                     nRet = SYSINFO_RC_UNSUPPORTED;
                  }
                  else
                  {
                     if (GetAttributeValue((ATA_SMART_VALUES *)pResult->bBuffer,
                                           (BYTE)nAttr, pValue, ATTR_TYPE_HEX_STRING))
                        nRet = SYSINFO_RC_SUCCESS;
                  }
               }
               else
               {
                  nRet = SYSINFO_RC_UNSUPPORTED;
               }
               break;
            case _T('T'):   // Temperature
               if (GetAttributeValue((ATA_SMART_VALUES *)pResult->bBuffer, 0xC2,
                                     pValue, ATTR_TYPE_BYTE))
               {
                  nRet = SYSINFO_RC_SUCCESS;
               }
               break;
            case _T('S'):   // SMART status
               if ((((IDEREGS *)pResult->bBuffer)->bCylHighReg == SMART_CYL_HI) &&
                   (((IDEREGS *)pResult->bBuffer)->bCylLowReg == SMART_CYL_LOW))
               {
                  ret_int(pValue, 0);  // Status is OK
               }
               else if ((((IDEREGS *)pResult->bBuffer)->bCylHighReg == 0x2C) &&
                        (((IDEREGS *)pResult->bBuffer)->bCylLowReg == 0xF4))
               {
                  ret_int(pValue, 1);  // Status is BAD
               }
               else
               {
                  ret_int(pValue, 2);  // Status is UNKNOWN
               }
               nRet = SYSINFO_RC_SUCCESS;
               break;
            case _T('M'):   // Model
#if defined UNICODE
				memcpy(pbValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->model, 40); // has bug
				pbValue[41] = 0;
               MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)pbValue, -1, pValue, 41);
               StrStrip(pValue);
#else
			   memcpy(pValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->model, 40); //
               pValue[41] = 0;
               StrStrip(pValue);
#endif
               nRet = SYSINFO_RC_SUCCESS;
               break;
            case _T('N'):   // Serial number
#if defined UNICODE
			   memcpy(pbValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->serial_no, 20); // has bug
			   pbValue[21] = 0;
               MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)pbValue, -1, pValue, 21);
               StrStrip(pValue); 
#else
			   memcpy(pValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->serial_no, 20); //
               pValue[21] = 0;
               StrStrip(pValue);
#endif
               nRet = SYSINFO_RC_SUCCESS;
               break;
            case _T('F'):   // Firmware
#if defined UNICODE
				memcpy(pbValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->fw_rev, 8); // has bug
			   pbValue[8] = 0;
               MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)pbValue, -1, pValue, 8);
               StrStrip(pValue); 
#else
				memcpy(pValue, ((ATA_IDENTIFY_DEVICE_DATA *)pResult->bBuffer)->fw_rev, 8);  //
               pValue[8] = 0;
               StrStrip(pValue);
#endif
				nRet = SYSINFO_RC_SUCCESS;
               break;
            default:
               nRet = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }

      free(pResult);
      CloseHandle(hDevice);
   }

   return nRet;
}
