/* 
** nxhwid - diplay local system hardware ID
** Copyright (C) 2006-2020 Raden Solutions
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

NETXMS_EXECUTABLE_HEADER(nxhwid)

#ifdef _WIN32

/**
 * SMBIOS header
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
 * SMBIOS reader - Windows
 */
static BYTE *BIOSReader(size_t *size)
{
   BYTE *buffer = (BYTE *)MemAlloc(16384);
   UINT rc = GetSystemFirmwareTable('RSMB', 0, buffer, 16384);
   if (rc > 16384)
   {
      buffer = (BYTE *)realloc(buffer, rc);
      rc = GetSystemFirmwareTable('RSMB', 0, buffer, rc);
   }
   if (rc == 0)
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(_T("smbios"), 3, _T("Call to GetSystemFirmwareTable failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      free(buffer);
      return NULL;
   }

   BiosHeader *header = reinterpret_cast<BiosHeader*>(buffer);
   BYTE *bios = (BYTE *)MemAlloc(header->length);
   memcpy(bios, header->tables, header->length);
   *size = header->length;
   MemFree(buffer);

   return bios;
}

#elif defined(__sun)

/**
 * SMBIOS reader - Solaris
 */
static BYTE *BIOSReader(size_t *biosSize)
{
   UINT32 fileSize;
   BYTE *data = LoadFileA("/dev/smbios", &fileSize);
   if (data == NULL)
      return NULL;

   // Check SMBIOS header
   if (memcmp(data, "_SM_", 4))
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Invalid SMBIOS header (anchor string not found)"));
      free(data);
      return NULL;  // not a valid SMBIOS signature
   }

   UINT32 offset = *reinterpret_cast<UINT32*>(data + 0x18);
   UINT32 dataSize = *reinterpret_cast<UINT16*>(data + 0x16);
   if (dataSize + offset > fileSize)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Invalid SMBIOS header (offset=%08X data_size=%04X file_size=%04X)"),
            offset, dataSize, fileSize);
      free(data);
      return NULL;
   }

   BYTE *biosData = (BYTE *)malloc(dataSize);
   memcpy(biosData, data + offset, dataSize);
   free(data);
   *biosSize = dataSize;
   return biosData;
}

#else

/**
 * SMBIOS reader - Linux
 */
static BYTE *BIOSReader(size_t *size)
{
   UINT32 fsize;
   BYTE *bios = LoadFileA("/sys/firmware/dmi/tables/DMI", &fsize);
   *size = fsize;
   return bios;
}

#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);
   SMBIOS_Parse(BIOSReader);
   
   int rc;
   BYTE hwid[HARDWARE_ID_LENGTH];
   if (GetSystemHardwareId(hwid))
   {
      TCHAR buffer[128];
      _tprintf(_T("%s\n"), BinToStr(hwid, sizeof(hwid), buffer));
      rc = 0;
   }
   else
   {
      _tprintf(_T("ERROR\n"));
      rc = 1;
   }
	
   return rc;
}
