/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
 * Processor types
 */
static const char *s_processorTypes[] =
{
   "Unknown",
   "Other",
   "Unknown",
   "CPU",
   "FPU",
   "DSP",
   "GPU"
};

/**
 * Processor types
 */
static const char *s_processorFamilies[] =
{
   "Unknown",
   "Other",
   "Unknown",
   "8086",
   "80286",
   "Intel386",
   "Intel486",
   "8087",
   "80287",
   "80387",
   "80487",
   "Intel Pentium",
   "Pentium Pro",
   "Pentium® II",
   "Pentium MMX",
   "Intel Celeron",
   "Pentium II Xeon",
   "Pentium III",
   "M1",
   "M2",
   "Intel Celeron M",
   "Intel Pentium 4 HT",
   "Unknown",  // 0x16
   "Unknown",  // 0x17
   "AMD Duron",
   "AMD K5",
   "AMD K6",
   "AMD K6-2",
   "AMD K6-3",
   "AMD Athlon",
   "AMD29000",
   "AMD K6-2+",
   "Power PC",
   "Power PC 601",
   "Power PC 603",
   "Power PC 603+",
   "Power PC 604",
   "Power PC 620",
   "Power PC x704",
   "Power PC 750",
   "Intel Core Duo",
   "Intel Core Duo mobile",
   "Intel Core Solo mobile",
   "Intel Atom",
   "Intel Core M" // 0x2C
};

/**
 * Processor information
 */
struct Processor
{
   char socket[32];
   const char *type;
   const char *family;
   char manufacturer[64];
   char version[64];
   UINT16 maxSpeed;
   UINT16 currentSpeed;
   char serial[32];
   char partNumber[32];
   int cores;
   int threads;
   UINT16 handle;
};

/**
 * Battery information
 */
struct Battery
{
   char name[64];
   char chemistry[32];
   UINT32 capacity;
   UINT16 voltage;
   char location[64];
   char manufacturer[64];
   char manufactureDate[32];
   char serial[32];
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
static StructArray<Processor> s_processors;
static StructArray<Battery> s_batteries;

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
 * Handler for battery parameters
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_BatteryParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instanceText[64];
   if (!AgentGetParameterArg(cmd, 1, instanceText, 64))
      return SYSINFO_RC_UNSUPPORTED;

   Battery *b = NULL;
   UINT16 instance = static_cast<UINT16>(_tcstol(instanceText, NULL, 0));
   for (int i = 0; i < s_batteries.size(); i++)
   {
      if (s_batteries.get(i)->handle == instance)
      {
         b = s_batteries.get(i);
         break;
      }
   }

   if (b == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch (*arg)
   {
      case 'C':
         ret_mbstring(value, b->chemistry);
         break;
      case 'c':
         ret_uint(value, b->capacity);
         break;
      case 'D':
         ret_mbstring(value, b->manufactureDate);
         break;
      case 'L':
         ret_mbstring(value, b->location);
         break;
      case 'M':
         ret_mbstring(value, b->manufacturer);
         break;
      case 'N':
         ret_mbstring(value, b->name);
         break;
      case 's':
         ret_mbstring(value, b->serial);
         break;
      case 'V':
         ret_uint(value, b->voltage);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

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
 * Handler for processor parameters
 */
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ProcessorParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instanceText[64];
   if (!AgentGetParameterArg(cmd, 1, instanceText, 64))
      return SYSINFO_RC_UNSUPPORTED;

   Processor *proc = NULL;
   UINT16 instance = static_cast<UINT16>(_tcstol(instanceText, NULL, 0));
   for(int i = 0; i < s_processors.size(); i++)
   {
      if (s_processors.get(i)->handle == instance)
      {
         proc = s_processors.get(i);
         break;
      }
   }

   if (proc == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'C':
         ret_uint(value, proc->cores);
         break;
      case 'c':
         ret_uint(value, proc->currentSpeed);
         break;
      case 'F':
         ret_mbstring(value, proc->family);
         break;
      case 'M':
         ret_mbstring(value, proc->manufacturer);
         break;
      case 'm':
         ret_uint(value, proc->maxSpeed);
         break;
      case 'P':
         ret_mbstring(value, proc->partNumber);
         break;
      case 'S':
         ret_mbstring(value, proc->socket);
         break;
      case 's':
         ret_mbstring(value, proc->serial);
         break;
      case 'T':
         ret_mbstring(value, proc->type);
         break;
      case 't':
         ret_uint(value, proc->threads);
         break;
      case 'V':
         ret_mbstring(value, proc->version);
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
      case 'B':   // batteries
         for (int i = 0; i < s_batteries.size(); i++)
         {
            value->add(s_batteries.get(i)->handle);
         }
         break;
      case 'M':   // memory devices
         for(int i = 0; i < s_memoryDevices.size(); i++)
         {
            value->add(s_memoryDevices.get(i)->handle);
         }
         break;
      case 'P':   // processors
         for(int i = 0; i < s_processors.size(); i++)
         {
            value->add(s_processors.get(i)->handle);
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
      case 'B':   // batteries
         value->addColumn(_T("HANDLE"), DCI_DT_INT, _T("Handle"), true);
         value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
         value->addColumn(_T("LOCATION"), DCI_DT_STRING, _T("Location"));
         value->addColumn(_T("CAPACITY"), DCI_DT_UINT, _T("Capacity"));
         value->addColumn(_T("VOLTAGE"), DCI_DT_UINT, _T("Voltage"));
         value->addColumn(_T("CHEMISTRY"), DCI_DT_STRING, _T("Chemistry"));
         value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
         value->addColumn(_T("MANUFACTURE_DATE"), DCI_DT_STRING, _T("Manufacture Date"));
         value->addColumn(_T("SERIAL_NUMBER"), DCI_DT_STRING, _T("Serial Number"));
         for(int i = 0; i < s_batteries.size(); i++)
         {
            value->addRow();
            value->set(0, s_batteries.get(i)->handle);
            value->set(1, s_batteries.get(i)->name);
            value->set(2, s_batteries.get(i)->location);
            value->set(3, s_batteries.get(i)->capacity);
            value->set(4, s_batteries.get(i)->voltage);
            value->set(5, s_batteries.get(i)->chemistry);
            value->set(6, s_batteries.get(i)->manufacturer);
            value->set(7, s_batteries.get(i)->manufactureDate);
            value->set(8, s_batteries.get(i)->serial);
         }
         break;
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
         for (int i = 0; i < s_memoryDevices.size(); i++)
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
      case 'P':   // processors
         value->addColumn(_T("HANDLE"), DCI_DT_INT, _T("Handle"), true);
         value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
         value->addColumn(_T("FAMILY"), DCI_DT_STRING, _T("Family"));
         value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"));
         value->addColumn(_T("SOCKET"), DCI_DT_STRING, _T("Socket"));
         value->addColumn(_T("CORES"), DCI_DT_UINT, _T("Cores"));
         value->addColumn(_T("THREADS"), DCI_DT_UINT, _T("Threads"));
         value->addColumn(_T("MAX_SPEED"), DCI_DT_UINT64, _T("Max Speed"));
         value->addColumn(_T("CURR_SPEED"), DCI_DT_UINT64, _T("Current Speed"));
         value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
         value->addColumn(_T("PART_NUMBER"), DCI_DT_STRING, _T("Part Number"));
         value->addColumn(_T("SERIAL_NUMBER"), DCI_DT_STRING, _T("Serial Number"));
         for(int i = 0; i < s_processors.size(); i++)
         {
            value->addRow();
            value->set(0, s_processors.get(i)->handle);
            value->set(1, s_processors.get(i)->type);
            value->set(2, s_processors.get(i)->family);
            value->set(3, s_processors.get(i)->version);
            value->set(4, s_processors.get(i)->socket);
            value->set(5, s_processors.get(i)->cores);
            value->set(6, s_processors.get(i)->threads);
            value->set(7, s_processors.get(i)->maxSpeed);
            value->set(8, s_processors.get(i)->currentSpeed);
            value->set(9, s_processors.get(i)->manufacturer);
            value->set(10, s_processors.get(i)->partNumber);
            value->set(11, s_processors.get(i)->serial);
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
   memset(buffer, 0, size);

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
   GetStringByIndex(t, BYTE_AT(t, 0x07), s_baseboardSerialNumber, sizeof(s_baseboardSerialNumber));

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
 * Parse processor information (table type 4)
 */
static void ParseProcessorInformation(TableHeader *t)
{
   BYTE status = BYTE_AT(t, 0x18);
   if ((status & 0x40) == 0)
      return;  // Empty socket

   Processor proc;
   memset(&proc, 0, sizeof(Processor));

   proc.handle = WORD_AT(t, 0x02);
   GetStringByIndex(t, BYTE_AT(t, 0x04), proc.socket, sizeof(proc.socket));
   GetStringByIndex(t, BYTE_AT(t, 0x07), proc.manufacturer, sizeof(proc.manufacturer));
   GetStringByIndex(t, BYTE_AT(t, 0x10), proc.version, sizeof(proc.version));
   proc.maxSpeed = WORD_AT(t, 0x14);
   proc.currentSpeed = WORD_AT(t, 0x16);

   int type = BYTE_AT(t, 0x05);
   proc.type = s_processorTypes[((type > 0) && (type <= 0x06)) ? type : 0];

   int family = BYTE_AT(t, 0x06);
   proc.family = s_processorFamilies[((family > 0) && (family <= 0x2C)) ? family : 0];

   if (t->fixedLength >= 0x23)   // version 2.3+
   {
      GetStringByIndex(t, BYTE_AT(t, 0x20), proc.serial, sizeof(proc.serial));
      GetStringByIndex(t, BYTE_AT(t, 0x22), proc.partNumber, sizeof(proc.partNumber));
      if (t->fixedLength >= 0x28)   // version 2.5+
      {
         proc.cores = BYTE_AT(t, 0x23);
         proc.threads = BYTE_AT(t, 0x25);
         if (t->fixedLength >= 0x30)   // version 3.0+
         {
            if (proc.cores == 255)
               proc.cores = WORD_AT(t, 0x2A);
            if (proc.threads == 255)
               proc.threads = WORD_AT(t, 0x2E);
         }
      }
   }

   s_processors.add(&proc);
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
 * Decode battery chemistry enum
 */
void DecodeBatteryChemistry(BYTE code, char *buffer, size_t size)
{
   static const char *values[] = {
      "Other", "Unknown", "Lead Acid", "NiCd", "NiMH", "Li-ion", "Zinc-air", "LiPo"
   };
   if ((code < 1) || (code > 8))
      code = 2;
   strlcpy(buffer, values[code - 1], size);
}

/**
 * Parse battery information (type 22)
 */
static void ParseBatteryInformation(TableHeader *t)
{
   Battery b;
   b.handle = WORD_AT(t, 0x02);
   GetStringByIndex(t, BYTE_AT(t, 0x04), b.location, sizeof(b.location));
   GetStringByIndex(t, BYTE_AT(t, 0x05), b.manufacturer, sizeof(b.manufacturer));
   GetStringByIndex(t, BYTE_AT(t, 0x08), b.name, sizeof(b.name));
   b.voltage = WORD_AT(t, 0x0C);
   if (t->fixedLength >= 0x16)   // 2.2+
   {
      if (BYTE_AT(t, 0x07) == 0)
         snprintf(b.serial, sizeof(b.serial), "%04X", WORD_AT(t, 0x10));
      else
         GetStringByIndex(t, BYTE_AT(t, 0x07), b.serial, sizeof(b.serial));

      if (BYTE_AT(t, 0x06) == 0)
      {
         UINT32 d = WORD_AT(t, 0x12);
         snprintf(b.manufactureDate, sizeof(b.manufactureDate), "%04d.%02d.%02d", 
            (d >> 9) + 1980, (d >> 5) & 0x000F, d & 0x001F);
      }
      else
      {
         GetStringByIndex(t, BYTE_AT(t, 0x06), b.manufactureDate, sizeof(b.manufactureDate));
      }

      if (BYTE_AT(t, 0x09) == 2)
         GetStringByIndex(t, BYTE_AT(t, 0x14), b.chemistry, sizeof(b.chemistry));
      else
         DecodeBatteryChemistry(BYTE_AT(t, 0x09), b.chemistry, sizeof(b.chemistry));

      b.capacity = static_cast<UINT32>(WORD_AT(t, 0x0A)) * static_cast<UINT32>(BYTE_AT(t, 0x15));
   }
   else
   {
      GetStringByIndex(t, BYTE_AT(t, 0x06), b.manufactureDate, sizeof(b.manufactureDate));
      GetStringByIndex(t, BYTE_AT(t, 0x07), b.serial, sizeof(b.serial));
      DecodeBatteryChemistry(BYTE_AT(t, 0x09), b.chemistry, sizeof(b.chemistry));
      b.capacity = WORD_AT(t, 0x0A);
   }
   s_batteries.add(&b);
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
         case 4:
            ParseProcessorInformation(curr);
            break;
         case 11:
            ParseOEMStrings(curr);
            break;
         case 17:
            ParseMemoryDeviceInformation(curr);
            break;
         case 22:
            ParseBatteryInformation(curr);
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
