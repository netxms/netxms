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
** File: smbios.cpp
**
**/

#include "libnxagent.h"

#define DEBUG_TAG _T("smbios")

/**
 * Memory device form factor table
 */
static const char *s_memoryFormFactors[] =
{
   "Unknown",
   "Other",
   "Unknown",
   "SIMM",
   "SIP",
   "Chip",
   "DIP",
   "ZIP",
   "Proprietary card",
   "DIMM",
   "TSOP",
   "Row of chips",
   "RIMM",
   "SO-DIMM",
   "SRIMM",
   "FB-DIMM"
};

/**
 * Memory device type table
 */
static const char *s_memoryTypes[] =
{
   "Unknown",
   "Other",
   "Unknown",
   "DRAM",
   "EDRAM",
   "VRAM",
   "SRAM",
   "RAM",
   "ROM",
   "FLASH",
   "EEPROM",
   "FEPROM",
   "EPROM",
   "CDRAM",
   "3DRAM",
   "SDRAM",
   "SGRAM",
   "RDRAM",
   "DDR",
   "DDR2",
   "DDR2 FB-DIMM",
   "Type 15h",
   "Type 16h",
   "Type 17h",
   "DDR3",
   "FBD2",
   "DDR4",
   "LPDDR",
   "LPDDR2",
   "LPDDR3",
   "LPDDR4",
   "Logical non-volatile device"
};

/**
 * Memory device information
 */
struct MemoryDevice
{
   UINT64 size;
   const char *formFactor;
   const char *type;
   char location[64];
   char bank[64];
   UINT32 maxSpeed;
   UINT32 configuredSpeed;
   char manufacturer[64];
   char serial[32];
   char partNumber[32];
   UINT16 handle;
};

/**
 * BIOS and system information
 */
static char s_baseboardManufacturer[128] = "";
static char s_baseboardProduct[128] = "";
static char s_baseboardSerialNumber[128] = "";
static char s_baseboardType[32] = "";
static char s_baseboardVersion[64] = "";
static WORD s_biosAddress = 0;
static char s_biosDate[16] = "";
static char s_biosVendor[128] = "";
static char s_biosVersion[64] = "";
static char s_hardwareManufacturer[128] = "";
static char s_hardwareProduct[128] = "";
static char s_hardwareSerialNumber[128] = "";
static char s_hardwareVersion[64] = "";
static char s_systemWakeUpEvent[32] = "Unknown";
static char *s_oemStrings[64];
static StructArray<MemoryDevice> s_memoryDevices;

/**
 * Get hardware manufacturer
 */
const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetHardwareManufacturer()
{
   return s_hardwareManufacturer;
}

/**
 * Get hardware product name
 */
const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetHardwareProduct()
{
   return s_hardwareProduct;
}

/**
 * Get OEM strings
 */
LIBNXAGENT_EXPORTABLE const char * const *SMBIOS_GetOEMStrings()
{
   return s_oemStrings;
}

#define RETURN_BIOS_DATA(p) do { if (*p == 0) return SYSINFO_RC_UNSUPPORTED; ret_mbstring(value, p); } while(0)

/**
 * Handler for memory device parameters
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_MemDevParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instanceText[64];
   if (!AgentGetParameterArg(cmd, 1, instanceText, 64))
      return SYSINFO_RC_UNSUPPORTED;

   MemoryDevice *md = NULL;
   UINT16 instance = static_cast<UINT16>(_tcstol(instanceText, NULL, 0));
   for(int i = 0; i < s_memoryDevices.size(); i++)
   {
      if (s_memoryDevices.get(i)->handle == instance)
      {
         md = s_memoryDevices.get(i);
         break;
      }
   }

   if (md == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'B':
         ret_mbstring(value, md->bank);
         break;
      case 'c':
         ret_uint(value, md->configuredSpeed);
         break;
      case 'F':
         ret_mbstring(value, md->formFactor);
         break;
      case 'L':
         ret_mbstring(value, md->location);
         break;
      case 'M':
         ret_mbstring(value, md->manufacturer);
         break;
      case 'm':
         ret_uint(value, md->maxSpeed);
         break;
      case 'P':
         ret_mbstring(value, md->partNumber);
         break;
      case 'S':
         ret_uint64(value, md->size);
         break;
      case 's':
         ret_mbstring(value, md->serial);
         break;
      case 'T':
         ret_mbstring(value, md->type);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for BIOS-related parameters
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch(arg[0])
   {
      case 'B':   // BIOS
         switch(arg[1])
         {
            case 'D':
               RETURN_BIOS_DATA(s_biosDate);
               break;
            case 'V':
               RETURN_BIOS_DATA(s_biosVersion);
               break;
            case 'v':
               RETURN_BIOS_DATA(s_biosVendor);
               break;
            default:
               return SYSINFO_RC_UNSUPPORTED;
         }
         break;
      case 'b':   // baseboard
         switch(arg[1])
         {
            case 'M':
               RETURN_BIOS_DATA(s_baseboardManufacturer);
               break;
            case 'P':
               RETURN_BIOS_DATA(s_baseboardProduct);
               break;
            case 'S':
               RETURN_BIOS_DATA(s_baseboardSerialNumber);
               break;
            case 'T':
               RETURN_BIOS_DATA(s_baseboardType);
               break;
            case 'V':
               RETURN_BIOS_DATA(s_baseboardVersion);
               break;
            default:
               return SYSINFO_RC_UNSUPPORTED;
         }
         break;
      case 'H':   // hardware
         switch(arg[1])
         {
            case 'M':
               RETURN_BIOS_DATA(s_hardwareManufacturer);
               break;
            case 'P':
               RETURN_BIOS_DATA(s_hardwareProduct);
               break;
            case 'S':
               RETURN_BIOS_DATA(s_hardwareSerialNumber);
               break;
            case 'V':
               RETURN_BIOS_DATA(s_hardwareVersion);
               break;
            default:
               return SYSINFO_RC_UNSUPPORTED;
         }
         break;
      case 'W':
         RETURN_BIOS_DATA(s_systemWakeUpEvent);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for BIOS-related lists
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ListHandler(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'M':   // memory devices
         for(int i = 0; i < s_memoryDevices.size(); i++)
         {
            value->add(s_memoryDevices.get(i)->handle);
         }
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for BIOS-related tables
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_TableHandler(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'M':   // memory devices
         value->addColumn(_T("HANDLE"), DCI_DT_INT, _T("Handle"), true);
         value->addColumn(_T("LOCATION"), DCI_DT_STRING, _T("Location"));
         value->addColumn(_T("BANK"), DCI_DT_STRING, _T("Bank"));
         value->addColumn(_T("FORM_FACTOR"), DCI_DT_STRING, _T("Form factor"));
         value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
         value->addColumn(_T("SIZE"), DCI_DT_UINT64, _T("Size"));
         value->addColumn(_T("MAX_SPEED"), DCI_DT_UINT64, _T("Max Speed"));
         value->addColumn(_T("CONF_SPEED"), DCI_DT_UINT64, _T("Configured Speed"));
         value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
         value->addColumn(_T("PART_NUMBER"), DCI_DT_STRING, _T("Part Number"));
         value->addColumn(_T("SERIAL_NUMBER"), DCI_DT_STRING, _T("Serial Number"));
         for(int i = 0; i < s_memoryDevices.size(); i++)
         {
            value->addRow();
            value->set(0, s_memoryDevices.get(i)->handle);
            value->set(1, s_memoryDevices.get(i)->location);
            value->set(2, s_memoryDevices.get(i)->bank);
            value->set(3, s_memoryDevices.get(i)->formFactor);
            value->set(4, s_memoryDevices.get(i)->type);
            value->set(5, s_memoryDevices.get(i)->size);
            value->set(6, s_memoryDevices.get(i)->maxSpeed);
            value->set(7, s_memoryDevices.get(i)->configuredSpeed);
            value->set(8, s_memoryDevices.get(i)->manufacturer);
            value->set(9, s_memoryDevices.get(i)->partNumber);
            value->set(10, s_memoryDevices.get(i)->serial);
         }
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

/**
 * BIOS table header
 */
struct TableHeader
{
   BYTE type;
   BYTE fixedLength;
   WORD handle;
};

#if defined(__HP_aCC)
#pragma pack
#elif defined(_AIX) && !defined(__GNUC__)
#pragma pack(pop)
#else
#pragma pack()
#endif

/**
 * Get BIOS string by index
 */
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

#define BYTE_AT(t, a)  (*(reinterpret_cast<BYTE*>(t) + a))
#define WORD_AT(t, a)  (*(reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(t) + a)))
#define DWORD_AT(t, a) (*(reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(t) + a)))

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
 * Parse base board information (table type 2)
 */
static void ParseBaseBoardInformation(TableHeader *t)
{
   GetStringByIndex(t, BYTE_AT(t, 0x04), s_baseboardManufacturer, sizeof(s_baseboardManufacturer));
   GetStringByIndex(t, BYTE_AT(t, 0x05), s_baseboardProduct, sizeof(s_baseboardProduct));
   GetStringByIndex(t, BYTE_AT(t, 0x06), s_baseboardVersion, sizeof(s_baseboardVersion));
   GetStringByIndex(t, BYTE_AT(t, 0x04), s_baseboardSerialNumber, sizeof(s_baseboardSerialNumber));

   switch(BYTE_AT(t, 0x0D))
   {
      case 0x02:
         strcpy(s_baseboardType, "Other");
         break;
      case 0x03:
         strcpy(s_baseboardType, "Server Blade");
         break;
      case 0x04:
         strcpy(s_baseboardType, "Connectivity Switch");
         break;
      case 0x05:
         strcpy(s_baseboardType, "System Management Module");
         break;
      case 0x06:
         strcpy(s_baseboardType, "Processor Module");
         break;
      case 0x07:
         strcpy(s_baseboardType, "I/O Module");
         break;
      case 0x08:
         strcpy(s_baseboardType, "Memory Module");
         break;
      case 0x09:
         strcpy(s_baseboardType, "Daughter Board");
         break;
      case 0x0A:
         strcpy(s_baseboardType, "Motherboard");
         break;
      case 0x0B:
         strcpy(s_baseboardType, "Processor/Memory Module");
         break;
      case 0x0C:
         strcpy(s_baseboardType, "Processor/IO Module");
         break;
      case 0x0D:
         strcpy(s_baseboardType, "Interconnect Board");
         break;
      default:
         strcpy(s_baseboardType, "Unknown");
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
* Parse memory device information (table type 17)
*/
static void ParseMemoryDeviceInformation(TableHeader *t)
{
   if (WORD_AT(t, 0x0C) == 0)
      return;  // Empty memory slot

   MemoryDevice md;
   md.handle = WORD_AT(t, 0x02);

   WORD size = WORD_AT(t, 0x0C);
   if (size != 0xFFFF)
   {
      if (size == 0x7FFF)
      {
         // Use extended size field
         md.size = static_cast<UINT64>(DWORD_AT(t, 0x1C)) * _ULL(1024 * 1024);
      }
      else
      {
         if (size & 0x8000)   // size in KBytes if bit 15 is set
            md.size = static_cast<UINT64>(size & 0x7FFF) * _ULL(1024);
         else
            md.size = static_cast<UINT64>(size & 0x7FFF) * _ULL(1024 * 1024);
      }
   }
   else
   {
      md.size = 0;   // unknown
   }

   int ff = BYTE_AT(t, 0x0E);
   md.formFactor = s_memoryFormFactors[((ff > 0) && (ff <= 0x0F)) ? ff : 0];

   int type = BYTE_AT(t, 0x12);
   md.type = s_memoryTypes[((type > 0) && (type <= 0x1F)) ? type : 0];

   GetStringByIndex(t, BYTE_AT(t, 0x10), md.location, sizeof(md.location));
   GetStringByIndex(t, BYTE_AT(t, 0x11), md.bank, sizeof(md.bank));
   md.maxSpeed = WORD_AT(t, 0x15);
   GetStringByIndex(t, BYTE_AT(t, 0x17), md.manufacturer, sizeof(md.manufacturer));
   GetStringByIndex(t, BYTE_AT(t, 0x18), md.serial, sizeof(md.serial));
   GetStringByIndex(t, BYTE_AT(t, 0x1A), md.partNumber, sizeof(md.partNumber));
   md.configuredSpeed = WORD_AT(t, 0x20);

   s_memoryDevices.add(&md);
}

/**
 * Parse SMBIOS data
 */
bool LIBNXAGENT_EXPORTABLE SMBIOS_Parse(BYTE *(*reader)(size_t *size))
{
   memset(s_oemStrings, 0, sizeof(s_oemStrings));

   size_t size;
   BYTE *bios = reader(&size);
   if (bios == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("BIOS read failed"));
      return false;
   }

   TableHeader *curr = reinterpret_cast<TableHeader*>(bios);
   while(static_cast<size_t>(reinterpret_cast<BYTE*>(curr) - bios) < size)
   {
      switch(curr->type)
      {
         case 0:
            ParseBIOSInformation(curr);
            break;
         case 1:
            ParseSystemInformation(curr);
            break;
         case 2:
            ParseBaseBoardInformation(curr);
            break;
         case 11:
            ParseOEMStrings(curr);
            break;
         case 17:
            ParseMemoryDeviceInformation(curr);
            break;
         default:
            break;
      }
      // Scan for 0x00 0x00 after fixed part
      BYTE *p = (BYTE *)curr + curr->fixedLength;
      while(*((WORD*)p) != 0)
         p++;
      curr = (TableHeader *)(p + 2);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("System manufacturer: %hs"), s_hardwareManufacturer);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("System product name: %hs"), s_hardwareProduct);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("BIOS vendor: %hs"), s_biosVendor);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("BIOS version: %hs"), s_biosVersion);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("BIOS address: %04X"), s_biosAddress);

   free(bios);
   return true;
}
