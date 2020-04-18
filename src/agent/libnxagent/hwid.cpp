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
#include <sys/cfgodm.h>
#include <odmi.h>
#elif defined(__sun)
#include <sys/systeminfo.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <sys/sysctl.h>
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
 * Read attribute from sys0 device using ODM
 */
static bool ReadAttributeFromSysDevice(const char *attribute, char *buffer)
{
   char query[256];
   snprintf(query, 256, "name='sys0' and attribute='%s'", attribute);

   bool success = false;
   struct CuAt object;
   long rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, query, &object, ODM_FIRST), long);
   while((rc != 0) && (rc != -1))
   {
      // Sanity check - some versions of AIX return all attributes despite filter in odm_get_obj
      if (!strcmp(attribute, object.attribute))
      {
         strlcpy(buffer, object.value, INTERNAL_BUFFER_SIZE);
         success = true;
         break;
      }
      rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, query, &object, ODM_NEXT), long);
   }
   return success;
}

/**
 * Get hardware serial number - AIX
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   if (ReadAttributeFromSysDevice("systemid", buffer))
      return true;

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
   return ReadAttributeFromSysDevice("modelname", buffer);
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

#elif defined(_HPUX)

/**
 * Get hardware serial number - HP-UX
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   return confstr(_CS_MACHINE_MODEL, buffer, INTERNAL_BUFFER_SIZE) > 0;
}

/**
 * Get hardware product - Solaris
 */
static bool GetHardwareProduct(char *buffer)
{
   return confstr(_CS_MACHINE_SERIAL, buffer, INTERNAL_BUFFER_SIZE) > 0;
}

#elif defined(__APPLE__)

/**
 * Get hardware serial number - macOS
 */
static bool GetHardwareSerialNumber(char *buffer)
{
   bool success = false;
   io_service_t platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
   if (platformExpert)
   {
      CFTypeRef serial = IORegistryEntryCreateCFProperty(platformExpert, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
      if (serial != NULL)
      {
         success = CFStringGetCString((CFStringRef)serial, buffer, INTERNAL_BUFFER_SIZE, kCFStringEncodingUTF8);
         CFRelease(serial);
      }
      IOObjectRelease(platformExpert);
   }
   return success;
}

/**
 * Get hardware product - macOS
 */
static bool GetHardwareProduct(char *buffer)
{
   size_t len = INTERNAL_BUFFER_SIZE;
   int ret = sysctlbyname("hw.model", buffer, &len, NULL, 0);
   return (ret == 0) || (errno == ENOMEM);
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
 * Get hardware ID for local system. Provided buffer should be at least HARDWARE_ID_LENGTH bytes long.
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
