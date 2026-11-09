/*
** NetXMS - Network Management System
** Driver for Linux systems accessible via SSH
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: linux.cpp
**
**/

#include "unix.h"

/**
 * Get driver name
 */
const wchar_t *LinuxDriver::getName()
{
   return L"LINUX";
}

/**
 * Get kernel name reported by "uname -s"
 */
const char *LinuxDriver::getKernelName() const
{
   return "Linux";
}

/**
 * Copy UTF-8 string into wide character buffer of given size (in characters)
 */
static inline void CopyUTF8String(wchar_t *dst, const char *src, size_t size)
{
   size_t len = utf8_to_wchar(src, -1, dst, size - 1);
   dst[len] = 0;
}

/**
 * Check if value read from DMI is a placeholder put by firmware vendor
 */
static bool IsDMIPlaceholder(const char *value)
{
   static const char *placeholders[] = { "", "0", "None", "Not Specified", "Not Applicable", "To Be Filled By O.E.M.", "Default string", nullptr };
   for(int i = 0; placeholders[i] != nullptr; i++)
      if (!stricmp(value, placeholders[i]))
         return true;
   return false;
}

/**
 * Get hardware information from DMI data exposed in /sys/class/dmi/id (serial number is usually readable by root only)
 */
bool LinuxDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   ByteStream output;
   if (!executeCommand(context, "grep -s . /sys/class/dmi/id/sys_vendor /sys/class/dmi/id/product_name /sys/class/dmi/id/product_version /sys/class/dmi/id/product_serial /sys/class/dmi/id/product_sku", &output))
      return false;

   // Each line has format path:value
   bool success = false;
   char *context2 = nullptr;
   for(char *line = strtok_r(reinterpret_cast<char*>(const_cast<BYTE*>(output.buffer())), "\r\n", &context2); line != nullptr; line = strtok_r(nullptr, "\r\n", &context2))
   {
      char *value = strchr(line, ':');
      if (value == nullptr)
         continue;
      *value = 0;
      value++;
      TrimA(value);
      if (IsDMIPlaceholder(value))
         continue;

      const char *name = strrchr(line, '/');
      name = (name != nullptr) ? name + 1 : line;
      if (!strcmp(name, "sys_vendor"))
         CopyUTF8String(hwInfo->vendor, value, sizeof(hwInfo->vendor) / sizeof(wchar_t));
      else if (!strcmp(name, "product_name"))
         CopyUTF8String(hwInfo->productName, value, sizeof(hwInfo->productName) / sizeof(wchar_t));
      else if (!strcmp(name, "product_version"))
         CopyUTF8String(hwInfo->productVersion, value, sizeof(hwInfo->productVersion) / sizeof(wchar_t));
      else if (!strcmp(name, "product_serial"))
         CopyUTF8String(hwInfo->serialNumber, value, sizeof(hwInfo->serialNumber) / sizeof(wchar_t));
      else if (!strcmp(name, "product_sku"))
         CopyUTF8String(hwInfo->productCode, value, sizeof(hwInfo->productCode) / sizeof(wchar_t));
      else
         continue;
      success = true;
   }
   nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"LinuxDriver::getHardwareInformation(%s): vendor=\"%s\" product=\"%s\" version=\"%s\"",
         node->getName(), hwInfo->vendor, hwInfo->productName, hwInfo->productVersion);
   return success;
}

/**
 * Get virtualization type using systemd-detect-virt
 */
bool LinuxDriver::getVirtualizationType(DeviceContext *context, NObject *node, DriverData *driverData, VirtualizationType *vtype)
{
   ByteStream output;
   if (!executeCommand(context, "systemd-detect-virt", &output))
      return false;

   char virt[64];
   strlcpy(virt, reinterpret_cast<const char*>(output.buffer()), sizeof(virt));
   char *eol = strpbrk(virt, "\r\n");
   if (eol != nullptr)
      *eol = 0;
   TrimA(virt);
   nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"LinuxDriver::getVirtualizationType(%s): systemd-detect-virt returned \"%hs\"", node->getName(), virt);

   // Output is empty if systemd-detect-virt is not available
   if (virt[0] == 0)
      return false;

   if (!strcmp(virt, "none"))
   {
      *vtype = VTYPE_NONE;
      return true;
   }

   static const char *containers[] = { "openvz", "lxc", "lxc-libvirt", "systemd-nspawn", "docker", "podman", "rkt", "wsl", "proot", "pouch", nullptr };
   for(int i = 0; containers[i] != nullptr; i++)
   {
      if (!strcmp(virt, containers[i]))
      {
         *vtype = VTYPE_CONTAINER;
         return true;
      }
   }
   *vtype = VTYPE_FULL;
   return true;
}

/**
 * Parse JSON document at given position of command output and advance position past it.
 * Command output may contain several documents produced by consecutive commands.
 */
static json_t *ParseNextJsonDocument(const char **text)
{
   while (isspace(static_cast<unsigned char>(**text)))
      (*text)++;
   if (**text == 0)
      return nullptr;

   json_error_t error;
   json_t *json = json_loads(*text, JSON_DISABLE_EOF_CHECK, &error);
   if (json == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"LinuxDriver: cannot parse JSON output (%hs at position %d)", error.text, error.position);
      return nullptr;
   }
   *text += error.position;
   return json;
}

/**
 * Get interface type from link type and kind reported by "ip -d"
 */
static uint32_t InterfaceTypeFromLink(const char *linkType, const char *kind)
{
   if (!strcmp(linkType, "loopback"))
      return IFTYPE_SOFTWARE_LOOPBACK;
   if (!strcmp(linkType, "ppp"))
      return IFTYPE_PPP;
   if (!strcmp(linkType, "ether"))
   {
      if (!strcmp(kind, "vlan"))
         return IFTYPE_L2VLAN;
      if (!strcmp(kind, "bridge"))
         return IFTYPE_BRIDGE;
      if (!strcmp(kind, "bond"))
         return IFTYPE_IEEE8023ADLAG;
      return IFTYPE_ETHERNET_CSMACD;
   }
   if (!strcmp(linkType, "none") || !strcmp(linkType, "ipip") || !strcmp(linkType, "gre") || !strcmp(linkType, "sit"))
      return IFTYPE_TUNNEL;
   return IFTYPE_OTHER;
}

/**
 * Parse lines in format /sys/class/net/<name>/speed:<speed in Mbps> and call given callback
 * for each interface with known speed (speed is passed in bits per second)
 */
static void ParseInterfaceSpeeds(char *text, const std::function<void (const char*, uint64_t)>& callback)
{
   char *context = nullptr;
   for(char *line = strtok_r(text, "\r\n", &context); line != nullptr; line = strtok_r(nullptr, "\r\n", &context))
   {
      if (strncmp(line, "/sys/class/net/", 15))
         continue;
      char *name = line + 15;
      char *sep = strchr(name, '/');
      if ((sep == nullptr) || strncmp(sep, "/speed:", 7))
         continue;
      *sep = 0;
      int speed = strtol(sep + 7, nullptr, 10);
      if (speed > 0)
         callback(name, static_cast<uint64_t>(speed) * 1000000);
   }
}

/**
 * Get list of interfaces using "ip -j -d addr show"; interface speed is read from /sys/class/net/<name>/speed (in Mbps)
 */
InterfaceList *LinuxDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   ByteStream output;
   if (!executeCommand(context, "ip -j -d addr show; grep -s . /sys/class/net/*/speed", &output))
      return nullptr;

   const char *text = reinterpret_cast<const char*>(output.buffer());
   json_t *root = ParseNextJsonDocument(&text);
   if (!json_is_array(root))
   {
      json_decref(root);
      nxlog_debug_tag(DEBUG_TAG_UNIX, 5, L"LinuxDriver::getInterfaces(%s): invalid output from \"ip -j -d addr show\"", node->getName());
      return nullptr;
   }

   InterfaceList *ifList = new InterfaceList();
   size_t i;
   json_t *element;
   json_array_foreach(root, i, element)
   {
      uint32_t ifIndex = json_object_get_uint32(element, "ifindex", 0);
      const char *ifName = json_object_get_string_utf8(element, "ifname", nullptr);
      if ((ifIndex == 0) || (ifName == nullptr))
         continue;

      auto iface = new InterfaceInfo(ifIndex);
      CopyUTF8String(iface->name, ifName, MAX_DB_STRING);
      wcscpy(iface->description, iface->name);
      iface->mtu = json_object_get_uint32(element, "mtu", 0);

      json_t *linkInfo = json_object_get(element, "linkinfo");
      const char *kind = (linkInfo != nullptr) ? json_object_get_string_utf8(linkInfo, "info_kind", "") : "";
      iface->type = InterfaceTypeFromLink(json_object_get_string_utf8(element, "link_type", ""), kind);

      // Network devices backed by hardware (PCI, USB, etc.) report parent bus
      iface->isPhysicalPort = (json_object_get(element, "parentbus") != nullptr) && (iface->type == IFTYPE_ETHERNET_CSMACD);

      MacAddress mac = MacAddress::parse(json_object_get_string_utf8(element, "address", ""));
      if (mac.isValid() && (mac.length() == MAC_ADDR_LENGTH))
         memcpy(iface->macAddr, mac.value(), MAC_ADDR_LENGTH);

      size_t j;
      json_t *addrInfo;
      json_array_foreach(json_object_get(element, "addr_info"), j, addrInfo)
      {
         InetAddress addr = InetAddress::parse(json_object_get_string_utf8(addrInfo, "local", ""));
         if (addr.isValid())
         {
            addr.setMaskBits(json_object_get_int32(addrInfo, "prefixlen", addr.getMaskBits()));
            iface->ipAddrList.add(addr);
         }
      }

      ifList->add(iface);
   }
   json_decref(root);

   // Remaining output lines contain interface speeds
   ParseInterfaceSpeeds(const_cast<char*>(text),
      [ifList] (const char *name, uint64_t speed) -> void
      {
         wchar_t wname[MAX_DB_STRING];
         CopyUTF8String(wname, name, MAX_DB_STRING);
         for(int i = 0; i < ifList->size(); i++)
         {
            InterfaceInfo *iface = ifList->get(i);
            if (!wcscmp(iface->name, wname))
            {
               iface->speed = speed;
               break;
            }
         }
      });

   nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"LinuxDriver::getInterfaces(%s): %d interfaces", node->getName(), ifList->size());
   return ifList;
}

/**
 * Get ARP cache (IPv4 and IPv6 neighbors) using "ip -j neigh show"; interface names are mapped to indexes using "ip -j link show"
 */
shared_ptr<ArpCache> LinuxDriver::getArpCache(DeviceContext *context, DriverData *driverData)
{
   ByteStream output;
   if (!executeCommand(context, "ip -j link show; ip -j neigh show", &output))
      return shared_ptr<ArpCache>();

   const char *text = reinterpret_cast<const char*>(output.buffer());
   json_t *links = ParseNextJsonDocument(&text);
   json_t *neighbors = ParseNextJsonDocument(&text);
   if (!json_is_array(links) || !json_is_array(neighbors))
   {
      json_decref(links);
      json_decref(neighbors);
      nxlog_debug_tag(DEBUG_TAG_UNIX, 5, L"LinuxDriver::getArpCache: invalid output from \"ip -j\" commands");
      return shared_ptr<ArpCache>();
   }

   auto arpCache = make_shared<ArpCache>();
   size_t i;
   json_t *element;
   json_array_foreach(neighbors, i, element)
   {
      InetAddress addr = InetAddress::parse(json_object_get_string_utf8(element, "dst", ""));
      MacAddress mac = MacAddress::parse(json_object_get_string_utf8(element, "lladdr", ""));
      if (!addr.isValid() || !mac.isValid())
         continue;   // incomplete or failed entries do not have link layer address

      uint32_t ifIndex = 0;
      const char *dev = json_object_get_string_utf8(element, "dev", "");
      size_t j;
      json_t *link;
      json_array_foreach(links, j, link)
      {
         if (!strcmp(json_object_get_string_utf8(link, "ifname", ""), dev))
         {
            ifIndex = json_object_get_uint32(link, "ifindex", 0);
            break;
         }
      }
      arpCache->addEntry(addr, mac, ifIndex);
   }
   json_decref(links);
   json_decref(neighbors);

   nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"LinuxDriver::getArpCache: %d entries", arpCache->size());
   return arpCache;
}

/**
 * Create driver data
 */
void LinuxDriver::analyzeDevice(DeviceContext *context, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
      *driverData = new LinuxDriverData();
}

/**
 * Check if JSON array of strings contains given value
 */
static bool JsonArrayContains(json_t *array, const char *value)
{
   size_t i;
   json_t *element;
   json_array_foreach(array, i, element)
   {
      if (json_is_string(element) && !strcmp(json_string_value(element), value))
         return true;
   }
   return false;
}

/**
 * Read states of all interfaces using "ip -j link show" (flags and operational state) and /sys/class/net/<name>/speed
 */
bool LinuxDriver::readInterfaceStates(DeviceContext *context, StructArray<LinuxInterfaceState> *states)
{
   ByteStream output;
   if (!executeCommand(context, "ip -j link show; grep -s . /sys/class/net/*/speed", &output))
      return false;

   const char *text = reinterpret_cast<const char*>(output.buffer());
   json_t *root = ParseNextJsonDocument(&text);
   if (!json_is_array(root))
   {
      json_decref(root);
      nxlog_debug_tag(DEBUG_TAG_UNIX, 5, L"LinuxDriver::readInterfaceStates: invalid output from \"ip -j link show\"");
      return false;
   }

   size_t i;
   json_t *element;
   json_array_foreach(root, i, element)
   {
      LinuxInterfaceState *state = states->addPlaceholder();
      state->ifIndex = json_object_get_uint32(element, "ifindex", 0);
      strlcpy(state->name, json_object_get_string_utf8(element, "ifname", ""), sizeof(state->name));
      state->speed = 0;

      json_t *flags = json_object_get(element, "flags");
      state->adminState = JsonArrayContains(flags, "UP") ? IF_ADMIN_STATE_UP : IF_ADMIN_STATE_DOWN;

      // Kernel reports UNKNOWN for interfaces without operational state tracking (loopback, many virtual
      // interfaces); for those carrier flag LOWER_UP is used
      const char *operState = json_object_get_string_utf8(element, "operstate", "UNKNOWN");
      if (!strcmp(operState, "UP"))
         state->operState = IF_OPER_STATE_UP;
      else if (!strcmp(operState, "DOWN") || !strcmp(operState, "LOWERLAYERDOWN"))
         state->operState = IF_OPER_STATE_DOWN;
      else if (!strcmp(operState, "DORMANT"))
         state->operState = IF_OPER_STATE_DORMANT;
      else if (!strcmp(operState, "TESTING"))
         state->operState = IF_OPER_STATE_TESTING;
      else if (!strcmp(operState, "NOTPRESENT"))
         state->operState = IF_OPER_STATE_NOT_PRESENT;
      else
         state->operState = JsonArrayContains(flags, "LOWER_UP") ? IF_OPER_STATE_UP : IF_OPER_STATE_DOWN;
   }
   json_decref(root);

   ParseInterfaceSpeeds(const_cast<char*>(text),
      [states] (const char *name, uint64_t speed) -> void
      {
         for(int i = 0; i < states->size(); i++)
         {
            LinuxInterfaceState *state = states->get(i);
            if (!strcmp(state->name, name))
            {
               state->speed = speed;
               break;
            }
         }
      });
   return true;
}

/**
 * Interface states read from device are reused for this time (in milliseconds), so polling all interfaces of a node
 * requires single command execution
 */
#define INTERFACE_STATE_CACHE_TIME  10000

/**
 * Get interface state
 */
void LinuxDriver::getInterfaceState(DeviceContext *context, NObject *node, DriverData *driverData, uint32_t ifIndex, const wchar_t *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   *adminState = IF_ADMIN_STATE_UNKNOWN;
   *operState = IF_OPER_STATE_UNKNOWN;

   // Driver data can be missing if status poll runs before first configuration poll after server start
   LinuxDriverData *data = static_cast<LinuxDriverData*>(driverData);
   StructArray<LinuxInterfaceState> localStates;
   StructArray<LinuxInterfaceState> *states;
   if (data != nullptr)
   {
      data->mutex.lock();
      int64_t now = GetMonotonicClockTime();
      if (now - data->interfaceStateTimestamp >= INTERFACE_STATE_CACHE_TIME)
      {
         // Failure is cached as well, so unreachable device is not queried again for each interface
         data->interfaceStates.clear();
         readInterfaceStates(context, &data->interfaceStates);
         data->interfaceStateTimestamp = now;
      }
      states = &data->interfaceStates;
   }
   else
   {
      readInterfaceStates(context, &localStates);
      states = &localStates;
   }

   for(int i = 0; i < states->size(); i++)
   {
      LinuxInterfaceState *state = states->get(i);
      if (state->ifIndex == ifIndex)
      {
         *adminState = state->adminState;
         *operState = state->operState;
         if (state->speed != 0)
            *speed = state->speed;
         break;
      }
   }

   if (data != nullptr)
      data->mutex.unlock();
}
