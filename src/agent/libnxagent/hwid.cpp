/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: hwid.cpp
**
**/

#include "libnxagent.h"

#if defined(_WIN32)
#include <intrin.h>
#elif defined(_AIX)
#include <sys/utsname.h>
#elif HAVE_CPUID_H
#include <cpuid.h>
#endif

/**
 * Get hardware ID for local system. Provided buffer should be at least SHA1_DIGEST_SIZE bytes long.
 */
bool LIBNXAGENT_EXPORTABLE GetSystemHardwareId(BYTE *hwid)
{
   bool success = false;

   SHA1_STATE ctx;
   SHA1Init(&ctx);

   // Add CPU vendor to the hash if possible
#if HAVE_GET_CPUID
   unsigned int data[4];
   if (__get_cpuid(0, &data[0], &data[1], &data[2], &data[3]) == 1)
   {
      SHA1Update(&ctx, &data[1], 12);
   }
#elif defined(_WIN32)
   int data[4];
   __cpuid(data, 0);
   SHA1Update(&ctx, &data[1], 12);
#endif

   // Add hardware serial number
#if defined(_AIX)
   struct utsname un;
   if (uname(&un) == 0)
   {
      SHA1Update(&ctx, un.machine, strlen(un.machine));
      success = true;
   }
#else
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
#ifndef _WIN32
   char buffer[128];
   if (*hwSerial == 0)
   {
      // Attempt to read serial number from file (works on Raspberry Pi)
      int fh = _open("/sys/firmware/devicetree/base/serial-number", O_RDONLY);
      if (fh != -1)
      {
         char buffer[128];
         int bytes = _read(fh, buffer, 127);
         if (bytes > 0)
         {
            buffer[bytes] = 0;
            hwSerial = buffer;
         }
         _close(fh);
      }
   }
#endif
   if (*hwSerial != 0)
   {
      SHA1Update(&ctx, hwSerial, strlen(hwSerial));
      success = true;
   }
#endif

   // Add hardware product
   const char *hwProduct = SMBIOS_GetHardwareProduct();
   if (*hwProduct != 0)
   {
      SHA1Update(&ctx, hwProduct, strlen(hwProduct));
   }
   else
   {
      // Attempt to read model from file (works on Raspberry Pi)
      int fh = _open("/sys/firmware/devicetree/base/model", O_RDONLY);
      if (fh != -1)
      {
         char buffer[128];
         int bytes = _read(fh, buffer, 128);
         if (bytes > 0)
         {
            SHA1Update(&ctx, buffer, bytes);
         }
         _close(fh);
      }
   }

   // Add baseboard serial number
   const char *baseboardSerial = SMBIOS_GetBaseboardSerialNumber();
   if (*baseboardSerial != 0)
   {
      SHA1Update(&ctx, baseboardSerial, strlen(baseboardSerial));
      success = true;
   }

   SHA1Finish(&ctx, hwid);
   return success;
}
