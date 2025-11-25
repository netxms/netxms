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
** File: disk.cpp
**
**/

#include "winnt_subagent.h"

/**
 * Get physical disk numbers
 */
static bool GetPhysicalDiskNumbers(StringList *numbers)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
      return false;

   UINT32 index = 0;
   TCHAR name[256];
   DWORD size = 256;
   while(RegEnumValue(hKey, index++, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
   {
      if (!_tcscmp(name, _T("0")))
      {
         numbers->add(name);
      }
      else
      {
         TCHAR *eptr;
         INT32 v = _tcstol(name, &eptr, 10);
         if ((v > 0) && (*eptr == 0))
         {
            numbers->add(name);
         }
      }
      size = 256;
   }
   RegCloseKey(hKey);
   return true;
}

/**
 * Get size of given physical disk
 */
static bool GetDiskSize(const TCHAR *number, UINT64 *size)
{
   TCHAR path[128];
   _sntprintf(path, 128, _T("\\\\.\\PhysicalDrive%s"), number);
   HANDLE device = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if (device == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskSize: cannot open device %s (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   GET_LENGTH_INFORMATION li;
   DWORD bytes;
   bool success;
   if (DeviceIoControl(device, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &li, sizeof(li), &bytes, NULL))
   {
      *size = li.Length.QuadPart;
      success = true;
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskSize: DeviceIoControl on device %s failed (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
      success = false;
   }
   CloseHandle(device);
   return success;
}

/**
 * Get information for given physical disk
 */
static BOOL GetDiskInfo(const TCHAR *number, STORAGE_DEVICE_DESCRIPTOR *info)
{
   TCHAR path[128];
   _sntprintf(path, 128, _T("\\\\.\\PhysicalDrive%s"), number);
   HANDLE device = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if (device == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskInfo: cannot open device %s (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
      return FALSE;
   }

   STORAGE_PROPERTY_QUERY pq;
   pq.PropertyId = StorageDeviceProperty;
   pq.QueryType = PropertyStandardQuery;

   DWORD bytes;
   BOOL success = DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &pq, sizeof(pq), info, 4096, &bytes, NULL);
   if (!success)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetDiskInfo: DeviceIoControl on device %s failed (%s)"), path, GetSystemErrorText(GetLastError(), buffer, 1024));
   }
   CloseHandle(device);
   return success;
}

/**
 * Convert bus type to string
 */
const TCHAR *BusTypeToString(STORAGE_BUS_TYPE type, TCHAR *buffer)
{
   static const TCHAR *typeName[] =
   {
      _T("Unknown"),
      _T("SCSI"),
      _T("ATAPI"),
      _T("ATA"),
      _T("IEEE 1394"),
      _T("SSA"),
      _T("FiberChannel"),
      _T("USB"),
      _T("RAID"),
      _T("iSCSI"),
      _T("SAS"),
      _T("SATA"),
      _T("SD"),
      _T("MMC"),
      _T("Virtual"),
      _T("File Backed Virtual"),
      _T("Spaces"),
      _T("NVMe"),
      _T("SCM"),
      _T("UFS")
   };
   return ((type >= 0) && (type <= 0x13)) ? typeName[type] : _itot(type, buffer, 16);
}

/**
 * Convert device type to string
 */
const TCHAR *DeviceTypeToString(int type)
{
   static const TCHAR *typeName[] =
   {
      _T("Direct-access"),
      _T("Sequential-access"),
      _T("Printer"),
      _T("Processor"),
      _T("Write-once"),
      _T("CD/DVD-ROM"),
      _T("Scanner"),
      _T("Optical memory"),
      _T("Medium changer"),
      _T("Communications"),
      _T("Graphic arts pre-press"),
      _T("Graphic arts pre-press"),
      _T("Storage array controller"),
      _T("Enclosure"),
      _T("Simplified direct-access"),
      _T("Optical card reader/writer"),
      _T("Reserved"),
      _T("Object-based storage"),
      _T("Automation/drive interface"),
      _T("Security manager"),
      _T("Host managed zoned")
   };
   return ((type >= 0) && (type <= 0x14)) ? typeName[type] : _T("");
}

/**
 * Handler for Hardware.StorageDevices list
 */
LONG H_StorageDeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   return GetPhysicalDiskNumbers(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
* Handler for Hardware.StorageDevices table
*/
LONG H_StorageDeviceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   StringList diskNumbers;
   if (!GetPhysicalDiskNumbers(&diskNumbers))
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

   UINT64 size;
   TCHAR temp[64];
   char buffer[4096];
   STORAGE_DEVICE_DESCRIPTOR *dev = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
   for(int i = 0; i < diskNumbers.size(); i++)
   {
      if (!GetDiskSize(diskNumbers.get(i), &size))
         size = 0;   // Get size may fail if process privileges is not elevated

      if (!GetDiskInfo(diskNumbers.get(i), dev))
         return SYSINFO_RC_ERROR;

      value->addRow();
      value->set(0, diskNumbers.get(i));
      value->set(1, dev->DeviceType);
      value->set(2, DeviceTypeToString(dev->DeviceType));
      value->set(3, BusTypeToString(dev->BusType, temp));
      value->set(4, dev->RemovableMedia);
      value->set(5, size);
      value->set(6, (dev->VendorIdOffset != 0) ? &buffer[dev->VendorIdOffset] : "");
      value->set(7, (dev->ProductIdOffset != 0) ? &buffer[dev->ProductIdOffset] : "");
      value->set(8, (dev->ProductRevisionOffset != 0) ? &buffer[dev->ProductRevisionOffset] : "");
      value->set(9, (dev->SerialNumberOffset != 0) ? &buffer[dev->SerialNumberOffset] : "");
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Hardware.StorageDevice.Size parameters
 */
LONG H_StorageDeviceSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR number[32];
   if (!AgentGetParameterArg(cmd, 1, number, 32))
      return SYSINFO_RC_UNSUPPORTED;

   UINT64 size;
   if (!GetDiskSize(number, &size))
      return SYSINFO_RC_ERROR;

   ret_uint64(value, size);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Hardware.StorageDevice.* parameters
 */
LONG H_StorageDeviceInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR number[32];
   if (!AgentGetParameterArg(cmd, 1, number, 32))
      return SYSINFO_RC_UNSUPPORTED;

   char buffer[4096];
   STORAGE_DEVICE_DESCRIPTOR *dev = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
   if (!GetDiskInfo(number, dev))
      return SYSINFO_RC_ERROR;

   TCHAR temp[64];
   switch (*arg)
   {
      case 'B':
         ret_string(value, BusTypeToString(dev->BusType, temp));
         break;
      case 'M':
         ret_mbstring(value, (dev->VendorIdOffset != 0) ? &buffer[dev->VendorIdOffset] : "");
         break;
      case 'P':
         ret_mbstring(value, (dev->ProductIdOffset != 0) ? &buffer[dev->ProductIdOffset] : "");
         break;
      case 'R':
         ret_mbstring(value, (dev->ProductRevisionOffset != 0) ? &buffer[dev->ProductRevisionOffset] : "");
         break;
      case 'r':
         ret_int(value, dev->RemovableMedia);
         break;
      case 'S':
         ret_mbstring(value, (dev->SerialNumberOffset != 0) ? &buffer[dev->SerialNumberOffset] : "");
         break;
      case 'T':
         ret_int(value, dev->DeviceType);
         break;
      case 't':
         ret_string(value, DeviceTypeToString(dev->DeviceType));
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}
