/* 
** NetXMS - Network Management System
** SMBIOS reader implementation for Windows
** Copyright (C) 2004-2024 Raden Solutions
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
** File: windows.cpp
**/

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
 * BIOS reader
 */
static BYTE *SMBIOS_Reader(size_t *size)
{
   BYTE *buffer = MemAllocArrayNoInit<BYTE>(16384);
   UINT rc = GetSystemFirmwareTable('RSMB', 0, buffer, 16384);
   if (rc > 16384)
   {
      buffer = MemRealloc(buffer, rc);
      rc = GetSystemFirmwareTable('RSMB', 0, buffer, rc);
   }
   if (rc == 0)
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(_T("smbios"), 3, _T("Call to GetSystemFirmwareTable failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      MemFree(buffer);
      return nullptr;
   }

   BiosHeader *header = reinterpret_cast<BiosHeader*>(buffer);
   BYTE *bios = MemAllocArrayNoInit<BYTE>(header->length);
   memcpy(bios, header->tables, header->length);
   *size = header->length;
   MemFree(buffer);

   return bios;
}
