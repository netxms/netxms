/* 
** NetXMS - Network Management System
** SMBIOS reader implementation for FreeBSD
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
** File: freebsd.cpp
**/

#include <kenv.h>

/**
 * Scan memory for SMBIOS header
 */
static off_t ScanMemory()
{
   int fd = _open("/dev/mem", O_RDONLY);
   if (fd == -1)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot open /dev/mem (%s)"), _tcserror(errno));
      return -1;
   }

   if (_lseek(fd, 0xF0000, SEEK_SET) == -1)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot seek /dev/mem to beginning of BIOS memory (%s)"), _tcserror(errno));
      close(fd);
      return -1;
   }

   bool found = false;
   off_t offset = 0;
   for(; offset < 0x10000; offset += 16)
   {
      BYTE buffer[16];
      if (_read(fd, buffer, 16) < 16)
         break;
      if (!memcmp(buffer, "_SM_", 4) || !memcmp(buffer, "_SM3_", 5))
      {
         found = true;
	 break;
      }
   }

   _close(fd);
   return found ? offset + 0xF0000 : -1;
}

/**
 * SMBIOS reader
 */
static BYTE *SMBIOS_Reader(size_t *size)
{
   off_t offset;
   char addrstr[KENV_MVALLEN + 1];
   if (kenv(KENV_GET, "hint.smbios.0.mem", addrstr, sizeof(addrstr)) > 0)
   {
      offset = static_cast<off_t>(strtoull(addrstr, nullptr, 0));
      nxlog_debug_tag(_T("smbios"), 5, _T("SMBIOS offset obtained from kernel environment: %lld"), static_cast<long long>(offset));
   }
   else
   {
      offset = ScanMemory();
      if (offset == -1)
      {
         nxlog_debug_tag(_T("smbios"), 5, _T("SMBIOS offset not found"));
         return nullptr;
      }
      nxlog_debug_tag(_T("smbios"), 5, _T("SMBIOS offset obtained from memory scan: %lld"), static_cast<long long>(offset));
   }

   int fd = _open("/dev/mem", O_RDONLY);
   if (fd == -1)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot open /dev/mem (%s)"), _tcserror(errno));
      return nullptr;
   }

   if (_lseek(fd, offset, SEEK_SET) == -1)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot seek /dev/mem to offset %lld (%s)"),
            static_cast<long long>(offset), _tcserror(errno));
      _close(fd);
      return nullptr;
   }

   BYTE biosHeader[32];
   if (_read(fd, biosHeader, 32) < 20)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot read at least 20 bytes from /dev/mem at offset %lld (%s)"),
            static_cast<long long>(offset), _tcserror(errno));
      _close(fd);
      return nullptr;
   }

   size_t dataSize;
   if (!memcmp(biosHeader, "_SM_", 4))
   {
      dataSize = *reinterpret_cast<uint16_t*>(&biosHeader[0x16]);
      offset = *reinterpret_cast<uint32_t*>(&biosHeader[0x18]);
   }
   else if (!memcmp(biosHeader, "_SM3_", 5))
   {
      dataSize = *reinterpret_cast<uint32_t*>(&biosHeader[0x0C]);
      offset = *reinterpret_cast<uint64_t*>(&biosHeader[0x10]);
   }
   else
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Invalid SMBIOS signature at offset %lld"), static_cast<long long>(offset));
      _close(fd);
      return nullptr;
   }

   if (_lseek(fd, offset, SEEK_SET) == -1)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot seek /dev/mem to offset %lld (%s)"),
            static_cast<long long>(offset), _tcserror(errno));
      _close(fd);
      return nullptr;
   }

   auto bios = MemAllocArray<BYTE>(dataSize);
   ssize_t bytes = _read(fd, bios, dataSize);
   if (bytes <= 0)
   {
      nxlog_debug_tag(_T("smbios"), 3, _T("Cannot read SMBIOS structures from /dev/mem at offset %lld (%s)"),
            static_cast<long long>(offset), _tcserror(errno));
      MemFreeAndNull(bios);
   }
   else
   {
      nxlog_debug_tag(_T("smbios"), 5, _T("Read %lld bytes of SMBIOS tables from /dev/mem"), static_cast<long long>(bytes));
   }
   _close(fd);
   
   *size = bytes;
   return bios;
}
