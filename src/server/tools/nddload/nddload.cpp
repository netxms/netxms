/* 
** nddload - command line tool for network device driver testing
** Copyright (C) 2013-2024 Raden Solutions
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
** File: nddload.cpp
**
**/

#include <nms_util.h>
#include <nxsnmp.h>
#include <nxsrvapi.h>
#include <nddrv.h>
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nddload)

/**
 * Driver data
 */
static DriverData *s_driverData = nullptr;
static NObject s_object;
static TCHAR *s_driverName = nullptr;
static ObjectArray<NetworkDeviceDriver> *s_drivers = nullptr;

/**
 * Options
 */
static bool s_showAccessPoints = false;
static bool s_showInterfaces = false;
static bool s_showRadioInterfaces = false;
static bool s_showVLANs = false;
static bool s_showWirelessStations = false;

/**
 * Access point state to text
 */
static const TCHAR *ApStateToText(AccessPointState state)
{
   switch(state)
   {
      case AP_DOWN:
         return _T("DOWN");
      case AP_UNPROVISIONED:
         return _T("UNPROVISIONED");
      case AP_UP:
         return _T("UP");
      default:
         return _T("UNKNOWN");
   }
}

/**
 * Print access points reported by wireless switch
 */
static void PrintAccessPoints(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<AccessPointInfo> *apInfo = driver->getAccessPoints(transport, &s_object, s_driverData);
   if (apInfo == nullptr)
   {
      _tprintf(_T("\nAccess point list is not available\n"));
      return;
   }

   _tprintf(_T("\nAccess points:\n"));
   for (int i = 0; i < apInfo->size(); i++)
   {
      AccessPointInfo *info = apInfo->get(i);

      TCHAR buff[64];
      _tprintf(
         _T("   %s\n")
         _T("      MAC address ... %s\n")
         _T("      State ......... %s\n")
         _T("      Model ......... %s\n")
         _T("      Serial ........ %s\n"),
         info->getName(),
         info->getMacAddr().toString(buff),
         ApStateToText(info->getState()),
         info->getModel(),
         info->getSerial());
      const StructArray<RadioInterfaceInfo>& interfaces = info->getRadioInterfaces();
      _tprintf(_T("      Radio Interfaces:\n"));
      for (int j = 0; j < interfaces.size(); j++)
      {
         RadioInterfaceInfo *rif = interfaces.get(j);
         _tprintf(
            _T("         %s\n")
            _T("            Index ...... %u\n")
            _T("            ifIndex .... %u\n")
            _T("            BSSID ...... %s\n")
            _T("            SSID ....... %s\n")
            _T("            Channel .... %u\n")
            _T("            Power mW ... %d\n")
            _T("            Power dBm .. %d\n"),
            rif->name, rif->index, rif->ifIndex, MACToStr(rif->bssid, buff), rif->ssid, rif->channel, rif->powerMW, rif->powerDBm);
      }
   }
   delete apInfo;
}

/**
 * Print radio interfaces points reported by wireless access point
 */
static void PrintRadioInterfaces(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   StructArray<RadioInterfaceInfo> *radios = driver->getRadioInterfaces(transport, &s_object, s_driverData);
   if (radios == nullptr)
   {
      _tprintf(_T("\nRadio interface list is not available\n"));
      return;
   }

   _tprintf(_T("Radio Interfaces:\n"));
   for (int i = 0; i < radios->size(); i++)
   {
      RadioInterfaceInfo *rif = radios->get(i);
      TCHAR buff[64];
      _tprintf(
         _T("   %s\n")
         _T("      Index ...... %u\n")
         _T("      ifIndex .... %u\n")
         _T("      BSSID ...... %s\n")
         _T("      SSID ....... %s\n")
         _T("      Channel .... %u\n")
         _T("      Power mW ... %d\n")
         _T("      Power dBm .. %d\n"),
         rif->name, rif->index, rif->ifIndex, MACToStr(rif->bssid, buff), rif->ssid, rif->channel, rif->powerMW, rif->powerDBm);
   }
   delete radios;
}

/**
 * Print wireless stations (clients) reported by wireless switch
 */
static void PrintWirelessStations(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<WirelessStationInfo> *wsInfo = driver->getWirelessStations(transport, &s_object, s_driverData);
   if (wsInfo == nullptr)
   {
      _tprintf(_T("\nWireless station list is not available\n"));
      return;
   }

   _tprintf(_T("\nWireless stations:\n"));
   for (int i = 0; i < wsInfo->size(); i++)
   {
      WirelessStationInfo *info = wsInfo->get(i);

      TCHAR macBuff[64];
      TCHAR ipBuff[64];
      _tprintf(_T("   %s - %-15s - vlan %-3d - rf %-2d - %s\n"),
            MACToStr(info->macAddr, macBuff),
            info->ipAddr.toString(ipBuff),
            info->vlan,
            info->rfIndex,
            info->ssid
            );
   }
   delete wsInfo;
}

/**
 * Print network interfaces
 */
static void PrintInterfaces(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   InterfaceList *ifList = driver->getInterfaces(transport, &s_object, s_driverData, true);
   if (ifList != nullptr)
   {
      if (ifList->size() > 0)
      {
         _tprintf(_T("\nInterfaces:\n"));
         for(int i = 0; i < ifList->size(); i++)
         {
            InterfaceInfo *iface = ifList->get(i);
            TCHAR macAddrText[64];
            _tprintf(_T("   %s\n")
                     _T("      Index ..........: %u\n")
                     _T("      Bridge port ....: %u\n")
                     _T("      Alias ..........: %s\n")
                     _T("      Description ....: %s\n")
                     _T("      Type ...........: %u\n")
                     _T("      Speed ..........: ") UINT64_FMT _T("\n")
                     _T("      MAC address ....: %s\n")
                     _T("      Is physical ....: %s\n"),
                  iface->name, iface->index, iface->bridgePort, iface->alias, iface->description, iface->type, iface->speed,
                  MACToStr(iface->macAddr, macAddrText), iface->isPhysicalPort ? _T("yes") : _T("no"));
            if (iface->isPhysicalPort)
            {
               TCHAR location[64];
               _tprintf(_T("      Location .......: %s\n"), iface->location.toString(location, 64));
            }
            if (!iface->ipAddrList.isEmpty())
            {
               _tprintf(_T("      IP address list:\n"));
               for(int j = 0; j < iface->ipAddrList.size(); j++)
               {
                  const InetAddress& ipAddr = iface->ipAddrList.get(j);
                  _tprintf(_T("         %s/%d\n"), ipAddr.toString().cstr(), ipAddr.getMaskBits());
               }
            }
         }
      }
      else
      {
         _tprintf(_T("\nInterface list is empty\n"));
      }
      delete ifList;
   }
   else
   {
      _tprintf(_T("\nInterface list is not available\n"));
   }
}

/**
 * Print VLANs
 */
static void PrintVLANs(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   VlanList *vlanList = driver->getVlans(transport, &s_object, s_driverData);
   if (vlanList != nullptr)
   {
      if (vlanList->size() > 0)
      {
         _tprintf(_T("\nVLANs:\n"));
         for(int i = 0; i < vlanList->size(); i++)
         {
            VlanInfo *vlan = vlanList->get(i);

            StringBuffer portList;
            TCHAR locationBuffer[64];
            for(int j = 0; j < vlan->getNumPorts(); j++)
            {
               const VlanPortInfo *port = vlan->getPort(j);
               if (!portList.isEmpty())
                  portList.append(_T(", "));
               switch(vlan->getPortReferenceMode())
               {
                  case VLAN_PRM_IFINDEX:
                  case VLAN_PRM_BPORT:
                     portList.append(port->portId);
                     break;
                  case VLAN_PRM_PHYLOC:
                     portList.append(port->location.toString(locationBuffer, 64));
                     break;
               }
            }

            _tprintf(_T("   %4d \"%s\" (%s)\n"), vlan->getVlanId(), vlan->getName(), portList.cstr());
         }
      }
      else
      {
         _tprintf(_T("\nVLAN list is empty\n"));
      }
      delete vlanList;
   }
   else
   {
      _tprintf(_T("\nVLAN list is not available\n"));
   }
}

/**
 * Print attribute
 */
static EnumerationCallbackResult PrintAttributeCallback(const TCHAR *key, const void *value, void *data)
{
   _tprintf(_T("   %s = %s\n"), key, (const TCHAR *)value);
   return _CONTINUE;
}

/**
 * Connect to device
 */
static bool ConnectToDevice(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   SNMP_ObjectId oid;
   uint32_t snmpErr = SnmpGet(transport->getSnmpVersion(), transport, { 1, 3, 6, 1, 2, 1, 1, 2, 0 }, &oid, 0, SG_OBJECT_ID_RESULT);
   if (snmpErr != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("Cannot get device OID (%s)\n"), SnmpGetErrorText(snmpErr));
      return false;
   }

   TCHAR oidText[256];
   _tprintf(_T("Device OID: %s\n"), oid.toString(oidText, 256));

   int pri = driver->isPotentialDevice(oid);
   if (pri <= 0)
   {
      _tprintf(_T("Driver is not suitable for device\n"));
      return false;
   }

   _tprintf(_T("Potential driver, priority %d\n"), pri);

   if (!driver->isDeviceSupported(transport, oid))
   {
      _tprintf(_T("Device is not supported by driver\n"));
      return false;
   }

   _tprintf(_T("Device is supported by driver\n"));

   driver->analyzeDevice(transport, oid, &s_object, &s_driverData);
   _tprintf(_T("Custom attributes after device analyze:\n"));
   auto attributes = s_object.getCustomAttributes();
   attributes->forEach(PrintAttributeCallback, nullptr);
   delete attributes;
   return true;
}

/**
 * Print device information
 */
static void PrintDeviceInformation(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   bool isWLC = false, isWAP = false;
   if (driver->isWirelessController(transport, &s_object, s_driverData))
   {
      _tprintf(_T("Device is a wireless controller\n"));
      _tprintf(_T("Cluster mode: %d\n"), driver->getClusterMode(transport, &s_object, s_driverData));
      isWLC = true;
   }
   else if (driver->isWirelessAccessPoint(transport, &s_object, s_driverData))
   {
      _tprintf(_T("Device is a wireless access point\n"));
      isWAP = true;
   }
   else
   {
      _tprintf(_T("Device is not a wireless controller nor access point\n"));
   }

   DeviceHardwareInfo hwInfo;
   memset(&hwInfo, 0, sizeof(hwInfo));
   if (driver->getHardwareInformation(transport, &s_object, s_driverData, &hwInfo))
   {
      _tprintf(_T("\nDevice hardware information:\n")
               _T("   Vendor ...........: %s\n")
               _T("   Product name .....: %s\n")
               _T("   Product code .....: %s\n")
               _T("   Product version ..: %s\n")
               _T("   Serial number ....: %s\n"),
               hwInfo.vendor, hwInfo.productName, hwInfo.productCode, hwInfo.productVersion, hwInfo.serialNumber);
   }
   else
   {
      _tprintf(_T("\nDevice hardware information is not available\n"));
   }

   if (s_showInterfaces)
      PrintInterfaces(driver, transport);

   if (s_showVLANs)
      PrintVLANs(driver, transport);

   if (isWLC && s_showAccessPoints)
      PrintAccessPoints(driver, transport);

   if (isWAP && s_showRadioInterfaces)
      PrintRadioInterfaces(driver, transport);

   if ((isWLC || isWAP) && s_showWirelessStations)
      PrintWirelessStations(driver, transport);
}

/**
 * Load driver and execute probes
 */
static void LoadDriver(const char *driver, const char *host, SNMP_Version snmpVersion, int snmpPort, const char *community, const shared_ptr<AgentConnection>& agentConnection)
{
   wchar_t errorText[1024];
   wchar_t *wdriver = WideStringFromMBString(driver);
   HMODULE hModule = DLOpen(wdriver, errorText);
   MemFree(wdriver);
   if (hModule == nullptr)
   {
      WriteToTerminalEx(L"Cannot load driver: %s\n", errorText);
      return;
   }

   SNMP_Transport *transport;
   if (agentConnection == nullptr)
   {
      transport = new SNMP_UDPTransport();
      static_cast<SNMP_UDPTransport*>(transport)->createUDPTransport(InetAddress::resolveHostName(host), snmpPort);
   }
   else
   {
      transport = new SNMP_ProxyTransport(agentConnection, InetAddress::resolveHostName(host), snmpPort);
   }
   transport->setSnmpVersion(snmpVersion);
   transport->setSecurityContext(new SNMP_SecurityContext(community));

   int *apiVersion = static_cast<int*>(DLGetSymbolAddr(hModule, "nddAPIVersion", errorText));
   ObjectArray<NetworkDeviceDriver> *(* CreateInstances)() = (ObjectArray<NetworkDeviceDriver> *(*)())DLGetSymbolAddr(hModule, "nddCreateInstances", errorText);
   if ((apiVersion != nullptr) && (CreateInstances != nullptr))
   {
      if (*apiVersion == NDDRV_API_VERSION)
      {
         s_drivers = CreateInstances();
         if (s_drivers != nullptr)
         {
            NetworkDeviceDriver *driver = s_drivers->get(0);
            for(int i = 0; i < s_drivers->size(); i++)
            {
               _tprintf(_T("Driver %s (version %s) loaded\n"), s_drivers->get(i)->getName(), s_drivers->get(i)->getVersion());
               if ((s_driverName != nullptr) && !_tcsicmp(s_driverName, s_drivers->get(i)->getName()))
               {
                  driver = s_drivers->get(i);
               }
            }

            _tprintf(_T("Connecting to host %hs:%d using community string %hs and driver %s\n"), host, snmpPort, community, driver->getName());

            if (ConnectToDevice(driver, transport))
            {
               PrintDeviceInformation(driver, transport);
            }
         }
         else
         {
            _tprintf(_T("Initialization of network device driver failed"));
            DLClose(hModule);
         }
      }
      else
      {
         _tprintf(_T("Driver API version mismatch (driver: %d; server: %d)\n"), *apiVersion, NDDRV_API_VERSION);
      }
   }
   else
   {
      _tprintf(_T("Required entry points missing\n"));
   }

   delete transport;
   DLClose(hModule);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   SNMP_Version snmpVersion = SNMP_VERSION_2C;
   int snmpPort = 161;
   const char *community = "public";
   wchar_t *agentAddress = nullptr;
   wchar_t *agentSecret = nullptr;
   uint32_t timeout = 2000;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   int ch;
   char *eptr;
   while((ch = getopt(argc, argv, "a:Ac:in:p:rs:t:v:Vw")) != -1)
   {
      switch(ch)
      {
         case 'a':   // proxy agent
            agentAddress = WideStringFromMBStringSysLocale(optarg);
            break;
         case 'A':   // Show access points
            s_showAccessPoints = true;
            break;
         case 'c':
            community = optarg;
            break;
         case 'i':   // Show interfaces
            s_showInterfaces = true;
            break;
         case 'n':
            s_driverName = WideStringFromMBStringSysLocale(optarg);
            break;
         case 'p':   // Port number
            snmpPort = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (snmpPort < 0) || (snmpPort > 65535))
            {
               _tprintf(_T("Invalid port number \"%hs\"\n"), optarg);
               return 1;
            }
            break;
         case 'r':   // Show radio interfaces
            s_showRadioInterfaces = true;
            break;
         case 's':   // agent secret
            agentSecret = WideStringFromMBStringSysLocale(optarg);
            break;
         case 't':   // timeout
            timeout = strtoul(optarg, nullptr, 10);
            if (timeout < 100)
               timeout = 100;
            break;
         case 'v':   // Version
            if (!strcmp(optarg, "1"))
            {
               snmpVersion = SNMP_VERSION_1;
            }
            else if (!stricmp(optarg, "2c"))
            {
               snmpVersion = SNMP_VERSION_2C;
            }
            else if (!stricmp(optarg, "3"))
            {
               snmpVersion = SNMP_VERSION_3;
            }
            else
            {
               _tprintf(_T("Invalid SNMP version %hs\n"), optarg);
               return 1;
            }
            break;
         case 'V':   // Show VLANs
            s_showVLANs = true;
            break;
         case 'w':   // Show wireless stations
            s_showWirelessStations = true;
            break;
         case '?':
            return 1;
         default:
            break;
      }
   }

   if (argc - optind < 2)
   {
      _tprintf(_T("Usage: nddload [-a agent] [-A] [-c community] [-i] [-n driver-name] [-p port] [-r] [-s agent-secret] [-t timeout] [-v snmp-version] [-V] [-w] driver-module device\n"));
      return 1;
   }

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   shared_ptr<AgentConnection> agentConnection;
   if (agentAddress != nullptr)
   {
      uint16_t port = AGENT_LISTEN_PORT;
      TCHAR *p = _tcsrchr(agentAddress, ':');
      if (p != nullptr)
      {
         *p = 0;
         port = static_cast<uint16_t>(_tcstoul(p + 1, nullptr, 10));
         if (port == 0)
         {
            _tprintf(_T("Invalid port number in agent connection string\n"));
            return 1;
         }
      }

      InetAddress addr = InetAddress::resolveHostName(agentAddress);
      if (!addr.isValidUnicast() && !addr.isLoopback())
      {
         _tprintf(_T("Invalid host name in agent connection string\n"));
         return 1;
      }

      agentConnection = make_shared<AgentConnection>(addr, port, agentSecret);
      agentConnection->setCommandTimeout(timeout + 1000);

      RSA_KEY key = RSAGenerateKey(2048);
      uint32_t rcc;
      if (!agentConnection->connect(key, &rcc))
      {
         RSAFree(key);
         _tprintf(_T("Cannot connect to agent at %s:%u (%s)\n"), addr.toString().cstr(), port, AgentErrorCodeToText(rcc));
         return 1;
      }
      RSAFree(key);

      _tprintf(_T("Connected to proxy agent at %s:%u\n"), addr.toString().cstr(), port);
   }

   SnmpSetDefaultTimeout(timeout);
   LoadDriver(argv[optind], argv[optind + 1], snmpVersion, snmpPort, community, agentConnection);

   MemFree(agentAddress);
   MemFree(s_driverName);
   return 0;
}
