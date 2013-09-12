#include <stdio.h>
#include <dlfcn.h>

#include <nxsnmp.h>
#include <nxsrvapi.h>
#include <nddrv.h>

void PrintAccessPoints(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<AccessPointInfo> *apInfo = driver->getAccessPoints(transport, NULL, NULL);
   if (apInfo != NULL)
   {
      printf("AccessPoints:\n");
      for (int i = 0; i < apInfo->size(); i++)
      {
         AccessPointInfo *info = apInfo->get(i);

         TCHAR buff[64];
         printf("\t%s - %9s - %s - %s\n",
               MACToStr(info->getMacAddr(), buff),
               info->getState() == AP_ADOPTED ? _T("adopted") : _T("unadopted"),
               info->getModel(),
               info->getSerial());
         ObjectArray<RadioInterfaceInfo> *interfaces = info->getRadioInterfaces();
         printf("\tRadio Interfaces:\n");
         for (int j = 0; j < interfaces->size(); j++)
         {
            RadioInterfaceInfo *interface = interfaces->get(j);
            printf("\t\t%2d - %s - %s\n",
                  interface->index,
                  MACToStr(interface->macAddr, buff),
                  interface->name);
         }
      }
   }
}

void PrintMobileUnits(NetworkDeviceDriver *driver, SNMP_Transport *transport)
{
   ObjectArray<WirelessStationInfo> *wsInfo = driver->getWirelessStations(transport, NULL, NULL);
   if (wsInfo != NULL)
   {
      printf("Wireless Stations:\n");
      for (int i = 0; i < wsInfo->size(); i++)
      {
         WirelessStationInfo *info = wsInfo->get(i);

         TCHAR macBuff[64];
         TCHAR ipBuff[64];
         printf("\t%s - %-15s - vlan %-3d - rf %-2d - %s\n",
               MACToStr(info->macAddr, macBuff),
               IpToStr(info->ipAddr, ipBuff),
               info->vlan,
               info->rfIndex,
               info->ssid
               );
      }
   }
}

int main(int argc, const char *argv[])
{
   void *handle = dlopen(".libs/symbol_ws.so", RTLD_NOW);

   if (handle == NULL)
   {
      perror("dlopen()");
   }
   
   SNMP_Transport *transport = new SNMP_UDPTransport;
   ((SNMP_UDPTransport *)transport)->createUDPTransport(NULL, inet_addr("10.22.100.31"), 161);
   transport->setSnmpVersion(SNMP_VERSION_1);
   transport->setSecurityContext(new SNMP_SecurityContext("netxms"));

   int *apiVersion = (int *)dlsym(handle, "nddAPIVersion");
   NetworkDeviceDriver *(* CreateInstance)() = (NetworkDeviceDriver *(*)())dlsym(handle, "nddCreateInstance");
   if ((apiVersion != NULL) && (CreateInstance != NULL))
   {
      if (*apiVersion == NDDRV_API_VERSION)
      {
         NetworkDeviceDriver *driver = CreateInstance();

         printf("Cluster Mode: %d\n", driver->getClusterMode(transport, NULL, NULL));

         PrintAccessPoints(driver, transport);
         PrintMobileUnits(driver, transport);
      }
   }

   delete transport;
   dlclose(handle);

   return 0;
}
