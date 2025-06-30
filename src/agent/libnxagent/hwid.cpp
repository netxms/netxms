/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
#if HAVE_CPUID_H
#include <cpuid.h>
#endif
#elif HAVE_CPUID_H
#include <cpuid.h>
#endif

#define INTERNAL_BUFFER_SIZE	256

#if defined(_WIN32)

/**
 * Get hardware serial number - Windows
 */
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial == 0)
      return false;
   strlcpy(buffer, hwSerial, size);
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

/**
 * Get unique machine ID - Windows
 */
static bool GetUniqueMachineId(char *buffer)
{
   uuid machineId = SMBIOS_GetHardwareUUID();
   if (machineId.isNull())
      return false;
   machineId.toStringA(buffer);
   return true;
}

#elif defined(_AIX)

/**
 * Read attribute from sys0 device using ODM
 */
static bool ReadAttributeFromSysDevice(const char *attribute, char *buffer, size_t size)
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
         strlcpy(buffer, object.value, size);
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
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
   if (!ReadAttributeFromSysDevice("systemid", buffer, size))
      return false;
   if (!strncmp(buffer, "IBM,02", 6))
      memmove(buffer, &buffer[6], strlen(&buffer[6]) + 1);
   return true;
}

/**
 * Get hardware product - AIX
 */
static bool GetHardwareProduct(char *buffer)
{
   return ReadAttributeFromSysDevice("modelname", buffer, INTERNAL_BUFFER_SIZE);
}

/**
 * Get unique machine ID - AIX
 */
static bool GetUniqueMachineId(char *buffer)
{
   struct utsname un;
   if (uname(&un) != 0)
      return false;
   strlcpy(buffer, un.machine, INTERNAL_BUFFER_SIZE);
   return true;
}

#elif defined(__sun)

/**
 * Get hardware serial number - Solaris
 */
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
#ifndef __sparc
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial != 0)
   {
      strlcpy(buffer, hwSerial, size);
      return true;
   }
#endif

   return sysinfo(SI_HW_SERIAL, buffer, size) > 0;
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


/**
 * Get unique machine ID - Solaris
 */
static bool GetUniqueMachineId(char *buffer)
{
#ifndef __sparc
   uuid machineId = SMBIOS_GetHardwareUUID();
   if (!machineId.isNull())
   {
      machineId.toStringA(buffer);
      return true;
   }
#endif
   return false;
}

#elif defined(_HPUX)

/**
 * Get hardware serial number - HP-UX
 */
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
   return confstr(_CS_MACHINE_MODEL, buffer, size) > 0;
}

/**
 * Get hardware product - HP-UX
 */
static bool GetHardwareProduct(char *buffer)
{
   return confstr(_CS_MACHINE_SERIAL, buffer, INTERNAL_BUFFER_SIZE) > 0;
}

/**
 * Get unique machine ID - HP-UX
 */
static bool GetUniqueMachineId(char *buffer)
{
   return confstr(_CS_PARTITION_IDENT, buffer, INTERNAL_BUFFER_SIZE) > 0;
}

#elif defined(__APPLE__)

/**
 * Get hardware serial number - macOS
 */
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
   bool success = false;
   io_service_t platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
   if (platformExpert)
   {
      CFTypeRef serial = IORegistryEntryCreateCFProperty(platformExpert, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
      if (serial != nullptr)
      {
         success = CFStringGetCString((CFStringRef)serial, buffer, size, kCFStringEncodingUTF8);
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
   int ret = sysctlbyname("hw.model", buffer, &len, nullptr, 0);
   return (ret == 0) || (errno == ENOMEM);
}

/**
 * Get unique machine ID - macOS
 */
static bool GetUniqueMachineId(char *buffer)
{
   return false;
}

#else

/**
 * Get hardware serial number - other platforms
 */
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size)
{
   const char *hwSerial = SMBIOS_GetHardwareSerialNumber();
   if (*hwSerial != 0)
   {
      strlcpy(buffer, hwSerial, size);
      return true;
   }

   // Attempt to read serial number from /sys/class/dmi/id/product_serial
   bool success = false;
   int fh = _open("/sys/class/dmi/id/product_serial", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, buffer, size - 1);
      if (bytes > 0)
      {
         buffer[bytes] = 0;
         TrimA(buffer);
         success = true;
      }
      _close(fh);
   }
   if (success)
      return true;

   // Attempt to read serial number from /sys/firmware/devicetree/base/serial-number (works on Raspberry Pi)
   fh = _open("/sys/firmware/devicetree/base/serial-number", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, buffer, size - 1);
      if (bytes > 0)
      {
         buffer[bytes] = 0;
         success = true;
      }
      _close(fh);
   }
   if (success)
      return true;

#if _OPENWRT
   // Attempt to read serial number from /etc/config/mnf_info (works on Teltonika modems and possibly other OpenWRT platforms)
   FILE *fp = fopen("/etc/config/mnf_info", "r");
   if (fp != nullptr)
   {
      char line[1024];
      while(!feof(fp))
      {
         if (fgets(line, 1024, fp) == nullptr)
            break;
         TrimA(line);
         if (!strncmp(line, "option serial_code '", 20))
         {
            char *p = strchr(&line[20], '\'');
            if (p != nullptr)
            {
               *p = 0;
               strlcpy(buffer, &line[20], size);
               success = true;
            }
            break;
         }
      }
      fclose(fp);
   }
   if (success)
      return true;

   // Attempt to read serial number from mnf_info command output (works on Teltonika modems and possibly other OpenWRT platforms)
   OutputCapturingProcessExecutor executor(_T("/sbin/mnf_info -s"), false);
   if (executor.execute())
   {
      if (executor.waitForCompletion(1000) && (executor.getExitCode() == 0))
      {
         const char *output = executor.getOutput();
         if (*output != 0)
         {
            const char *nl = strchr(output, '\n');
            if (nl != nullptr)
            {
               size_t len = nl - output;
               if (len > 0)
               {
                  if (len >= size)
                     len = size - 1;
                  memcpy(buffer, output, len);
                  buffer[len] = 0;
                  success = true;
               }
            }
            else
            {
               strlcpy(buffer, output, size);
               success = true;
            }
         }
      }
      else
      {
         executor.stop();
      }
   }
   if (success)
      return true;
#endif

   // Attempt to read serial number from /sys/devices/soc0/serial_number (works on Teltonika modems and possibly other Qualcomm SoC devices)
   fh = _open("/sys/devices/soc0/serial_number", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, buffer, size - 1);
      if (bytes > 0)
      {
         buffer[bytes] = 0;
         TrimA(buffer);
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

/**
 * Get unique machine ID - other platforms
 */
static bool GetUniqueMachineId(char *buffer)
{
   uuid machineId = SMBIOS_GetHardwareUUID();
   if (machineId.isNull())
      return false;
   machineId.toStringA(buffer);
   return true;
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
#elif defined(_WIN32) && !_M_ARM64
   int data[4];
   __cpuid(data, 0);
   SHA1Update(&ctx, &data[1], 12);
#endif

   // Add hardware serial number
   char buffer[INTERNAL_BUFFER_SIZE];
   if (GetHardwareSerialNumber(buffer, INTERNAL_BUFFER_SIZE))
   {
      SHA1Update(&ctx, buffer, strlen(buffer));
      success = true;
   }

   // Add hardware product
   if (GetHardwareProduct(buffer))
   {
      SHA1Update(&ctx, buffer, strlen(buffer));
   }

   // Add unique machine ID
   if (GetUniqueMachineId(buffer))
   {
      SHA1Update(&ctx, buffer, strlen(buffer));
      success = true;
   }

   // Add baseboard serial number
   const char *baseboardSerial = SMBIOS_GetBaseboardSerialNumber();
   if (*baseboardSerial != 0)
   {
      SHA1Update(&ctx, baseboardSerial, strlen(baseboardSerial));
      success = true;
   }

   SHA1Final(&ctx, hwid);
   return success;
}

/**
 * Get unique ID for local system (not necessarily hardware based). Provided buffer should be at least SYSTEM_ID_LENGTH bytes long.
 */
bool LIBNXAGENT_EXPORTABLE GetUniqueSystemId(BYTE *sysid)
{
   char id[INTERNAL_BUFFER_SIZE];
#if defined(_WIN32) || defined(_AIX)
   if (!GetUniqueMachineId(id))
      return false;
#else
   bool success = false;
   int fh = _open("/etc/machine-id", O_RDONLY);
   if (fh != -1)
   {
      int bytes = _read(fh, id, INTERNAL_BUFFER_SIZE - 1);
      if (bytes > 0)
      {
         id[bytes] = 0;
         success = true;
      }
      _close(fh);
   }

   if (!success)
      return false;
#endif

   SHA1_STATE ctx;
   SHA1Init(&ctx);
   SHA1Update(&ctx, "GetUniqueSystemId", 17);
   SHA1Update(&ctx, id, strlen(id));
   SHA1Final(&ctx, sysid);
   return true;
}
