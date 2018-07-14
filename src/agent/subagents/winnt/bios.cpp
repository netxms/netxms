/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: bios.cpp
**
**/

#include "winnt_subagent.h"

/**
 * BIOS and system information
 */
WORD s_biosAddress = 0;
char s_biosDate[16] = "";
char s_biosVendor[128] = "";
char s_biosVersion[64] = "";
char s_hardwareManufacturer[128] = "";
char s_hardwareProduct[128] = "";
char s_hardwareSerialNumber[128] = "";
char s_hardwareVersion[128] = "";
char s_systemWakeUpEvent[32] = "Unknown";
char *s_oemStrings[64];

/**
 * Get hardware manufacturer
 */
const char *GetHardwareManufacturer()
{
   return s_hardwareManufacturer;
}

/**
 * Get hardware product name
 */
const char *GetHardwareProduct()
{
   return s_hardwareProduct;
}

/**
 * Get OEM strings
 */
const char * const *GetOEMStrings()
{
   return s_oemStrings;
}

#define RETURN_BIOS_DATA(p) do { if (*p == 0) return SYSINFO_RC_UNSUPPORTED; ret_mbstring(value, p); } while(0)

/**
 * Handler for BIOS-related parameters
 */
LONG H_BIOSInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch (*arg)
   {
      case 'D':
         RETURN_BIOS_DATA(s_biosDate);
         break;
      case 'M':
         RETURN_BIOS_DATA(s_hardwareManufacturer);
         break;
      case 'P':
         RETURN_BIOS_DATA(s_hardwareProduct);
         break;
      case 'S':
         RETURN_BIOS_DATA(s_hardwareSerialNumber);
         break;
      case 'v':
         RETURN_BIOS_DATA(s_biosVendor);
         break;
      case 'V':
         RETURN_BIOS_DATA(s_biosVersion);
         break;
      case 'w':
         RETURN_BIOS_DATA(s_hardwareVersion);
         break;
      case 'W':
         RETURN_BIOS_DATA(s_systemWakeUpEvent);
         break;
   }
   return SYSINFO_RC_SUCCESS;
}

#pragma pack(1)

/**
 * BIOS header
 */
struct BiosHeader
{
   BYTE used20CallingMethod;
   BYTE majorVersion;
   BYTE minorVersion;
   BYTE dmiRevision;
   DWORD length;
   BYTE tables[1];
};

/**
 * BIOS table header
 */
struct TableHeader
{
   BYTE type;
   BYTE fixedLength;
   WORD handle;
};

#pragma pack()

static const char *GetStringByIndex(TableHeader *t, int index, char *buffer, size_t size)
{
   if (index < 1)
      return NULL;

   char *s = (char *)t + t->fixedLength;
   if (*((WORD*)s) == 0)
      return NULL;   // empty variable part
   
   while(index > 1)
   {
      char *p = s;
      while(*p != 0)
         p++;
      if (*((WORD*)p) == 0)
         return NULL;   // encounter end of table, string not found
      index--;
      s = p + 1;
   }

   if (buffer != NULL)
      strlcpy(buffer, s, size);
   return s;
}

#define BYTE_AT(t,a) (*(reinterpret_cast<BYTE*>(t) + a))
#define WORD_AT(t,a) (*(reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(t) + a)))

/**
 * Parse BIOS information (table type 0)
 */
static void ParseBIOSInformation(TableHeader *t)
{
   GetStringByIndex(t, BYTE_AT(t, 0x04), s_biosVendor, sizeof(s_biosVendor));
   GetStringByIndex(t, BYTE_AT(t, 0x05), s_biosVersion, sizeof(s_biosVersion));
   s_biosAddress = WORD_AT(t, 0x06);
   GetStringByIndex(t, BYTE_AT(t, 0x08), s_biosDate, sizeof(s_biosDate));
}

/**
 * Parse system information (table type 1)
 */
static void ParseSystemInformation(TableHeader *t)
{
   GetStringByIndex(t, BYTE_AT(t, 0x04), s_hardwareManufacturer, sizeof(s_hardwareManufacturer));
   GetStringByIndex(t, BYTE_AT(t, 0x05), s_hardwareProduct, sizeof(s_hardwareProduct));
   GetStringByIndex(t, BYTE_AT(t, 0x06), s_hardwareVersion, sizeof(s_hardwareVersion));
   GetStringByIndex(t, BYTE_AT(t, 0x07), s_hardwareSerialNumber, sizeof(s_hardwareSerialNumber));
   
   switch(BYTE_AT(t, 0x18))
   {
      case 1:
         strcpy(s_systemWakeUpEvent, "Other");
         break;
      case 3:
         strcpy(s_systemWakeUpEvent, "APM Timer");
         break;
      case 4:
         strcpy(s_systemWakeUpEvent, "Modem Ring");
         break;
      case 5:
         strcpy(s_systemWakeUpEvent, "LAN Remote");
         break;
      case 6:
         strcpy(s_systemWakeUpEvent, "Power Switch");
         break;
      case 7:
         strcpy(s_systemWakeUpEvent, "PCI PME#");
         break;
      case 8:
         strcpy(s_systemWakeUpEvent, "AC Power Restored");
         break;
   }
}

/**
* Parse OEM strings (table type 11)
*/
static void ParseOEMStrings(TableHeader *t)
{
   int count = BYTE_AT(t, 0x04);
   if (count > 63)
      count = 63;
   for (int i = 1; i <= count; i++)
   {
      const char *s = GetStringByIndex(t, i, NULL, 0);
      s_oemStrings[i - 1] = strdup(CHECK_NULL_EX_A(s));
   }
}

/**
 * Scan BIOS
 */
void ScanBIOS()
{
   memset(s_oemStrings, 0, sizeof(s_oemStrings));

   BYTE *buffer = (BYTE *)malloc(32768);
   UINT rc = GetSystemFirmwareTable('RSMB', 0, buffer, 32768);
   if (rc > 32768)
   {
      buffer = (BYTE *)realloc(buffer, rc);
      rc = GetSystemFirmwareTable('RSMB', 0, buffer, rc);
   }
   if (rc == 0)
   {
      TCHAR errorText[1024];
      AgentWriteDebugLog(3, _T("Call to GetSystemFirmwareTable failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      free(buffer);
      return;
   }

   BiosHeader *bios = (BiosHeader*)buffer;
   TableHeader *curr = (TableHeader *)bios->tables;
   while ((BYTE *)curr - bios->tables < bios->length)
   {
      switch (curr->type)
      {
         case 0:
            ParseBIOSInformation(curr);
            break;
         case 1:
            ParseSystemInformation(curr);
            break;
         case 11:
            ParseOEMStrings(curr);
            break;
         default:
            break;
      }
      // Scan for 0x00 0x00 after fixed part
      BYTE *p = (BYTE *)curr + curr->fixedLength;
      while (*((WORD*)p) != 0)
         p++;
      curr = (TableHeader *)(p + 2);
   }

   AgentWriteDebugLog(5, _T("System manufacturer: %hs"), s_hardwareManufacturer);
   AgentWriteDebugLog(5, _T("System product name: %hs"), s_hardwareProduct);
   AgentWriteDebugLog(5, _T("BIOS vendor: %hs"), s_biosVendor);
   AgentWriteDebugLog(5, _T("BIOS version: %hs"), s_biosVersion);
   AgentWriteDebugLog(5, _T("BIOS address: %04X"), s_biosAddress);

   free(buffer);
}
