/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2023 Raden Solutions
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
** File: hardware.cpp
**
**/

#include "linux_subagent.h"
#include <net/if.h>

/**
 * Executes lshw command with given options
 *
 * @returns lshw output as json_t array or nullptr on failure
 */
json_t* RunLSHW(const TCHAR* lshwOptions)
{
   TCHAR cmd[128];
   _sntprintf(cmd, 128, _T("lshw -json %s 2>/dev/null"), lshwOptions);

   OutputCapturingProcessExecutor pe(cmd);
   if (!pe.execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to execute lshw command"));
      return nullptr;
   }
   if (!pe.waitForCompletion(10000))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to execute lshw command: command timed out"));
      return nullptr;
   }

   json_error_t error;
   json_t* root = json_loads(pe.getOutput(), 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
      return nullptr;
   }

   if (!json_is_array(root))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON: top level value is not an array"));
      json_decref(root);
      return nullptr;
   }

   return root;
}

/**
 * Handler for Hardware.NetworkAdapters table
 */
LONG H_NetworkAdaptersTable(const TCHAR* cmd, const TCHAR* arg, Table* value, AbstractCommSession* session)
{
   // Run lshw
   json_t* root = RunLSHW(_T("-c network"));
   if (root == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"), true);
   value->addColumn(_T("PRODUCT"), DCI_DT_STRING, _T("Product"));
   value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
   value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
   value->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
   value->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
   value->addColumn(_T("SPEED"), DCI_DT_UINT64, _T("Speed"));
   value->addColumn(_T("AVAILABILITY"), DCI_DT_UINT, _T("Availability"));

   size_t size = json_array_size(root);
   for (size_t i = 0; i < size; i++)
   {
      json_t* data = json_array_get(root, i);
      if (!json_is_object(data))
         continue;

      const char *logicalName = json_string_value(json_object_get(data, "logicalname"));
      if ((logicalName != nullptr) && !strncmp(logicalName, "virbr", 5))
         continue;   // Ignore software bridge interfaces reported by lshw

      value->addRow();

      value->set(0, i);                                                                            // INDEX
      value->set(1, json_string_value(json_object_get(data, "product")));                          // PRODUCT
      value->set(2, json_string_value(json_object_get(data, "vendor")));                           // MANUFACTURER
      value->set(3, json_string_value(json_object_get(data, "description")));                      // DESCRIPTION
      json_t* wireless = json_object_get(json_object_get(data, "capabilities"), "wireless");
      value->set(4, wireless == nullptr ? _T("Ethernet 802.3") : _T("Wireless"));                  // TYPE
      value->set(5, json_string_value(json_object_get(data, "serial")));                           // MAC_ADDRESS
      const char* ifName = json_string_value(json_object_get(data, "logicalname"));
      value->set(6, ifName == nullptr ? 0 : if_nametoindex(ifName));                               // IF_INDEX
      value->set(7, json_object_get_int64(data, "capacity", 0));                                   // SPEED

      // AVAILABILITY
      json_t* disabled = json_object_get(data, "disabled");
      json_t* link = json_object_get_by_path_a(data, "configuration/link");
      if (json_is_true(disabled))
         value->set(8, 8); // Off Line
      else if (!json_is_string(link))
         value->set(8, 2); // Unknown
      else if (strcmp(json_string_value(link), "yes") == 0)
         value->set(8, 3); // Running/Full Power
      else
         value->set(8, 19); // Not Ready
   }
   json_decref(root);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Match bus type name in given lshw string. Returns nullptr if no known bus type found.
 * If new types are discovered they should be added here.
 */
static const TCHAR *MatchBusType(const char *text)
{
   if (text == nullptr)
      return nullptr;
   if (strcasestr(text, "scsi") != nullptr)
      return _T("SCSI");
   if (strcasestr(text, "sata") != nullptr)
      return _T("SATA");
   if (strcasestr(text, "ata") != nullptr)
      return _T("ATA");
   if (strcasestr(text, "sas") != nullptr)
      return _T("SAS");
   if (strcasestr(text, "nvme") != nullptr)
      return _T("NVMe");
   return nullptr;
}

/**
 * Get bus type of disk from lshw bus information or description.
 * Only run this for disks (type == 0)
 */
static const TCHAR *GetBusType(json_t *data)
{
   const TCHAR *busType = MatchBusType(json_object_get_string_utf8(data, "businfo", nullptr));
   if (busType == nullptr)
      busType = MatchBusType(json_object_get_string_utf8(data, "description", nullptr));
   return (busType != nullptr) ? busType : _T("Unknown");
}

/**
 * Parse JSON into Table for storage devices
 */
static void GetDataForStorageDevices(json_t *root, Table *value, int *curDevice)
{
   size_t size = json_array_size(root);
   for (size_t i = 0; i < size; i++)
   {
      json_t* data = json_array_get(root, i);
      if (!json_is_object(data))
         continue;

      value->addRow();
      value->set(0, (*curDevice)++); // NUMBER
      if (!strcmp(json_object_get_string_utf8(data, "class", ""), "storage"))
      {
         value->set(1, 12);                             // TYPE
         value->set(2, _T("Storage array controller")); // TYPE_DESCRIPTION
                                                        // BUS_TYPE = nullptr
      }
      else
      {
         value->set(1, 0);                   // TYPE
         value->set(2, _T("Direct-access")); // TYPE_DESCRIPTION
         value->set(3, GetBusType(data)); // BUS_TYPE
      }

      // REMOVABLE
      const char *driver = json_object_get_string_utf8(json_object_get(data, "configuration"), "driver", nullptr);
      value->set(4, ((driver != nullptr) && (strcasestr(driver, "usb") != nullptr)) ? 1 : 0);

      value->set(5, json_object_get_int64(data, "size", 0));                   // SIZE
      value->set(6, json_string_value(json_object_get(data, "vendor")));       // MANUFACTURER
      value->set(7, json_string_value(json_object_get(data, "product")));      // PRODUCT
      value->set(8, json_string_value(json_object_get(data, "version")));      // REVISION
      value->set(9, json_string_value(json_object_get(data, "serial")));       // SERIAL

      json_t* children = json_object_get(data, "children");
      if (children != nullptr && json_is_array(children))
         GetDataForStorageDevices(children, value, curDevice);
   }
}

/**
 * Handler for Hardware.StorageDevices table
 */
LONG H_StorageDeviceTable(const TCHAR* cmd, const TCHAR* arg, Table* value, AbstractCommSession* session)
{
   // Run lshw
   json_t* root = RunLSHW(_T("-c disk -c storage"));
   if (root == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NUMBER"), DCI_DT_UINT, _T("Number"), true);
   value->addColumn(_T("TYPE"), DCI_DT_UINT, _T("Type"));
   value->addColumn(_T("TYPE_DESCRIPTION"), DCI_DT_STRING, _T("Type description"));
   value->addColumn(_T("BUS_TYPE"), DCI_DT_STRING, _T("Bus type"));
   value->addColumn(_T("REMOVABLE"), DCI_DT_INT, _T("Removable"));
   value->addColumn(_T("SIZE"), DCI_DT_UINT64, _T("Size"));
   value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
   value->addColumn(_T("PRODUCT"), DCI_DT_STRING, _T("Product"));
   value->addColumn(_T("REVISION"), DCI_DT_STRING, _T("Revision"));
   value->addColumn(_T("SERIAL"), DCI_DT_STRING, _T("Serial number"));

   int i = 0;
   GetDataForStorageDevices(root, value, &i);

   json_decref(root);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Read content of file /sys/class/dmi/id/[fileName] into [value]
 *
 * @return SYSINFO_RC_XXX value
 */
static LONG GetHardwareInfoFromFile(const char* fileName, TCHAR* value)
{
   char path[MAX_PATH];
   snprintf(path, MAX_PATH, "/sys/class/dmi/id/%s", fileName);
   FILE* f = fopen(path, "r");
   if (f != nullptr)
   {
      char buffer[1024] = "";
      fgets(buffer, 1024, f);
      fclose(f);
      if ((buffer[0] != 0) && (buffer[0] != '\n'))
      {
         char* newLine = strchr(buffer, '\n');
         if (newLine != nullptr)
            *newLine = 0;
         ret_mbstring(value, buffer);
         return SYSINFO_RC_SUCCESS;
      }
   }
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for Hardware.System.* parameters
 */
LONG H_HardwareSystemInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   if (SMBIOS_ParameterHandler(param, arg, value, session) == SYSINFO_RC_SUCCESS)
      return SYSINFO_RC_SUCCESS;

   switch (arg[1])
   {
      case 'C':
         return GetHardwareInfoFromFile("product_sku", value);
      case 'M':
         return GetHardwareInfoFromFile("sys_vendor", value);
      case 'P':
         return GetHardwareInfoFromFile("product_name", value);
      case 'U':
         return GetHardwareInfoFromFile("product_uuid", value);
      case 'V':
         return GetHardwareInfoFromFile("product_version", value);
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
}
