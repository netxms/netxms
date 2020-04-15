/* 
** nddload - command line tool for network device driver testing
** Copyright (C) 2013-2020 Raden Solutions
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

NETXMS_EXECUTABLE_HEADER(nddload)

/**
 * Driver data
 */
static DriverData *s_driverData = NULL;
static NObject s_object;
static TCHAR *s_driverName = NULL;
static ObjectArray<NetworkDeviceDriver> *s_drivers = NULL;

/**
 * Print access points reported by wireless switch
 */
void PrintAccessPoints(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<AccessPointInfo> *apInfo = driver->getAccessPoints(transport, NULL, NULL);
   if (apInfo != NULL)
   {
      _tprintf(_T("\nAccessPoints:\n"));
      for (int i = 0; i < apInfo->size(); i++)
      {
         AccessPointInfo *info = apInfo->get(i);

         TCHAR buff[64];
         _tprintf(_T("   %s - %9s - %s - %s\n"),
               info->getMacAddr().toString(buff),
               info->getState() == AP_ADOPTED ? _T("adopted") : _T("unadopted"),
               info->getModel(),
               info->getSerial());
         const ObjectArray<RadioInterfaceInfo> *interfaces = info->getRadioInterfaces();
         _tprintf(_T("      Radio Interfaces:\n"));
         for (int j = 0; j < interfaces->size(); j++)
         {
            RadioInterfaceInfo *rif = interfaces->get(j);
            _tprintf(_T("         %2d - %s - %s\n"),
                  rif->index,
                  MACToStr(rif->macAddr, buff),
                  rif->name);
         }
      }
      delete apInfo;
   }
}

/**
 * Print mobile units (clients) reported by wireless switch
 */
void PrintMobileUnits(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<WirelessStationInfo> *wsInfo = driver->getWirelessStations(transport, NULL, NULL);
   if (wsInfo != NULL)
   {
      _tprintf(_T("\nWireless Stations:\n"));
      for (int i = 0; i < wsInfo->size(); i++)
      {
         WirelessStationInfo *info = wsInfo->get(i);

         TCHAR macBuff[64];
         TCHAR ipBuff[64];
         _tprintf(_T("   %s - %-15s - vlan %-3d - rf %-2d - %s\n"),
               MACToStr(info->macAddr, macBuff),
               IpToStr(info->ipAddr, ipBuff),
               info->vlan,
               info->rfIndex,
               info->ssid
               );
      }
      delete wsInfo;
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
   TCHAR oid[256];
   DWORD snmpErr = SnmpGet(transport->getSnmpVersion(), transport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, oid, sizeof(oid), SG_STRING_RESULT);
   if (snmpErr != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("Cannot get device OID (%s)\n"), SNMPGetErrorText(snmpErr));
      return false;
   }

   _tprintf(_T("Device OID: %s\n"), oid);

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
   attributes->forEach(PrintAttributeCallback, NULL);
   delete attributes;
   return true;
}

/**
 * Load driver and execute probes
 */
static void LoadDriver(const char *driver, const char *host, SNMP_Version snmpVersion, int snmpPort, const char *community)
{
   TCHAR errorText[1024];
#ifdef UNICODE
   WCHAR *wdriver = WideStringFromMBString(driver);
   HMODULE hModule = DLOpen(wdriver, errorText);
   free(wdriver);
#else
   HMODULE hModule = DLOpen(driver, errorText);
#endif
   if (hModule == NULL)
   {
      _tprintf(_T("Cannot load driver: %s\n"), errorText);
      return;
   }
   
   auto transport = new SNMP_UDPTransport;
   transport->createUDPTransport(InetAddress::resolveHostName(host), snmpPort);
   transport->setSnmpVersion(snmpVersion);
   transport->setSecurityContext(new SNMP_SecurityContext(community));

   int *apiVersion = (int *)DLGetSymbolAddr(hModule, "nddAPIVersion", errorText);
   ObjectArray<NetworkDeviceDriver> *(* CreateInstances)() = (ObjectArray<NetworkDeviceDriver> *(*)())DLGetSymbolAddr(hModule, "nddCreateInstances", errorText);
   if ((apiVersion != NULL) && (CreateInstances != NULL))
   {
      if (*apiVersion == NDDRV_API_VERSION)
      {
         s_drivers = CreateInstances();
         if (s_drivers != NULL)
         {
            NetworkDeviceDriver *driver = s_drivers->get(0);
            for(int i = 0; i < s_drivers->size(); i++)
            {
               _tprintf(_T("Driver %s (version %s) loaded\n"), s_drivers->get(i)->getName(), s_drivers->get(i)->getVersion());
               if ((s_driverName != NULL) && !_tcsicmp(s_driverName, s_drivers->get(i)->getName()))
               {
                  driver = s_drivers->get(i);
               }
            }

            _tprintf(_T("Connecting to host %hs:%d using community string %hs and driver %s\n"),
                     host, snmpPort, community, driver->getName());

            if (ConnectToDevice(driver, transport))
            {
               if (driver->isWirelessController(transport, &s_object, s_driverData))
               {
                  _tprintf(_T("Device is a wireless controller\n"));
                  _tprintf(_T("Cluster Mode: %d\n"), driver->getClusterMode(transport, NULL, NULL));

                  PrintAccessPoints(driver, transport);
                  PrintMobileUnits(driver, transport);
               }
               else
               {
                  _tprintf(_T("Device is not a wireless controller\n"));
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

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   int ch;
   char *eptr;
   while((ch = getopt(argc, argv, "c:n:p:v:")) != -1)
   {
      switch(ch)
      {
         case 'c':
            community = optarg;
            break;
         case 'n':
#ifdef UNICODE
            s_driverName = WideStringFromMBStringSysLocale(optarg);
#else
            s_driverName = strdup(optarg);
#endif
            break;
         case 'p':   // Port number
            snmpPort = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (snmpPort < 0) || (snmpPort > 65535))
            {
               _tprintf(_T("Invalid port number \"%hs\"\n"), optarg);
               return 1;
            }
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
         case '?':
            return 1;
         default:
            break;
      }
   }

   if (argc - optind < 2)
   {
      _tprintf(_T("Usage: nddload [-c community] [-n driverName] [-p port] [-v snmpVersion] driver device\n"));
      return 1;
   }

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   LoadDriver(argv[optind], argv[optind + 1], snmpVersion, snmpPort, community);

   MemFree(s_driverName);
   return 0;
}
