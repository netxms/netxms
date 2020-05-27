/* 
** NetXMS - Network Management System
** SMBIOS reader implementation for Solaris
** Copyright (C) 2004-2020 Raden Solutions
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
** File: solaris.cpp
**/

static BYTE *SMBIOS_Reader(size_t *biosSize)
{
   size_t fileSize;
   BYTE *data = LoadFileA("/dev/smbios", &fileSize);
   if (data == nullptr)
      return nullptr;

   // Check SMBIOS header
   if (memcmp(data, "_SM_", 4))
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Invalid SMBIOS header (anchor string not found)"));
      MemFree(data);
      return nullptr;  // not a valid SMBIOS signature
   }

   UINT32 offset = *reinterpret_cast<UINT32*>(data + 0x18);
   UINT32 dataSize = *reinterpret_cast<UINT16*>(data + 0x16);
   if (dataSize + offset > fileSize)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Invalid SMBIOS header (offset=%08X data_size=%04X file_size=%04X)"),
            offset, dataSize, fileSize);
      MemFree(data);
      return nullptr;
   }

   BYTE *biosData = (BYTE *)MemAlloc(dataSize);
   memcpy(biosData, data + offset, dataSize);
   MemFree(data);
   *biosSize = dataSize;
   return biosData;
}
