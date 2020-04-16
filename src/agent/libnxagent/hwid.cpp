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
#elif defined(__sun)
#include <sys/systeminfo.h>
#elif HAVE_CPUID_H
#include <cpuid.h>
#endif

#define INTERNAL_BUFFER_SIZE	256

#if defined(_WIN32)

/**
 * Get hardware serial number - Windows
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial == 0)
      return false;
   strlcpy(buffer, hwSerial, INTERNAL_BUFFER_SIZE);
   return true;
}

/**
 * Get hardware product - Windows
 */
static bool GetHardwareProduct(char *buffer)
{
   const char *hwProduct = SMBIOS_GetHardwareProduct();
   if (*hwProduct == 0)
      return false;
   strlcpy(buffer, hwProduct, INTERNAL_BUFFER_SIZE);
   return true;
}

#elif defined(_AIX)

/**
 * Get hardware serial number - AIX
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   struct utsname un;
   if (uname(&un) != 0)
      return false;
   strlcpy(buffer, un.machine, INTERNAL_BUFFER_SIZE);
   return true;
}

/**
 * Get hardware product - AIX
 */
static bool GetHardwareProduct(char *buffer)
{
   return false;
}

#elif defined(__sun)

/**
 * Get hardware serial number - Solaris
 */
static bool GetHardwareSerialNumber(char *buffer)
{
#ifndef __sparc
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial != 0)
   {
      strlcpy(buffer, hwSerial, INTERNAL_BUFFER_SIZE);
      return true;
   }
#endif

   return sysinfo(SI_HW_SERIAL, buffer, INTERNAL_BUFFER_SIZE) > 0;
}

/**
 * Get hardware product - Solaris
 */
static bool GetHardwareProduct(char *buffer)
{
#ifndef __sparc
   const char *hwProduct = SMBIOS_GetHardwareProduct();
   if (*hwProduct != 0)
   {
      strlcpy(buffer, hwProduct, INTERNAL_BUFFER_SIZE);
      return true;
   }
#endif

   int len = sysinfo(SI_HW_PROVIDER, buffer, INTERNAL_BUFFER_SIZE);
   if (len > 0)
      buffer[len++] = ' ';
   else
      len = 0;
   return sysinfo(SI_PLATFORM, &buffer[len], INTERNAL_BUFFER_SIZE - len) > 0;
}

#else

/**
 * Get hardware serial number - other platforms
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial != 0)
   {
      strlcpy(buffer, hwSerial, INTERNAL_BUFFER_SIZE);
      return true;
   }

   // Attempt to read serial number from file (works on Raspberry Pi)
   bool success = false;
   int fh = _open("/sys/firmware/devicetree/base/serial-number", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, buffer, INTERNAL_BUFFER_SIZE - 1);
      if (bytes > 0)
      {
         buffer[bytes] = 0;
         success = true;
      }
      _close(fh);
   }
   return success;
}

/**
 * Get hardware product - other platforms
 */
static bool GetHardwareProduct(char *buffer)
{
   const char *hwProduct = SMBIOS_GetHardwareProduct();
   if (*hwProduct != 0)
   {
      strlcpy(buffer, hwProduct, INTERNAL_BUFFER_SIZE);
      return true;
   }

   // Attempt to read model from file (works on Raspberry Pi)
   bool success = false;
   int fh = _open("/sys/firmware/devicetree/base/model", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, buffer, 128);
      if (bytes > 0)
      {
         buffer[bytes] = 0;
         success = true;
      }
      _close(fh);
   }
   return success;
}

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
   char buffer[INTERNAL_BUFFER_SIZE];
   if (GetHardwareSerialNumber(buffer))
   {
      SHA1Update(&ctx, buffer, strlen(buffer));
      success = true;
   }

   // Add hardware product
   if (GetHardwareProduct(buffer))
   {
      SHA1Update(&ctx, buffer, strlen(buffer));
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
