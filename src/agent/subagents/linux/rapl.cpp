/*
 ** NetXMS subagent for GNU/Linux
 ** Copyright (C) 2004-2024 Raden Solutions
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
 **/

#include "linux_subagent.h"

/**
 * Power zone
 */
struct PowerZone
{
   char *name;
   char *energyDataPath;
   PowerZone *next;     // next in flat list
   PowerZone *subzones; // first sub-zone
   PowerZone *snext;    // next sub-zone
   PowerZone *parent;   // parent zone
};

/**
 * Head of list of all registered power zones
 */
static PowerZone *s_powerZones = nullptr;

/**
 * Head of hierarchical tree of registered power zones
 */
static PowerZone *s_powerZoneTree = nullptr;

/**
 * Register new power zone
 */
static void RegisterPowerZone(const char *parentPath, const char *dirName, PowerZone *parentZone)
{
   char path[MAX_PATH];
   strcpy(path, parentPath);
   strcat(path, "/");
   strcat(path, dirName);

   PowerZone *zone = MemAllocStruct<PowerZone>();
   zone->parent = parentZone;

   size_t l = strlen(path);
   strcpy(&path[l], "/name");

   char name[256];
   if (!ReadLineFromFileA(path, name, 256))
      strlcpy(name, dirName, 256);
   if (parentZone != nullptr)
   {
      char fname[1024];
      strcpy(fname, parentZone->name);
      strlcat(fname, "/", 1024);
      strlcat(fname, name, 1024);
      zone->name = MemCopyStringA(fname);
   }
   else
   {
      zone->name = MemCopyStringA(name);
   }

   strcpy(&path[l], "/energy_uj");
   zone->energyDataPath = MemCopyStringA(path);

   // Insert into tree
   if (parentZone != nullptr)
   {
      zone->snext = parentZone->subzones;
      parentZone->subzones = zone;
   }
   else
   {
      zone->snext = s_powerZoneTree;
      s_powerZoneTree = zone;
   }

   // Insert into flat list
   zone->next = s_powerZones;
   s_powerZones = zone;

   // Scan sub-zones
   path[l] = 0;
   DIR *dir = opendir(path);
   if (dir != nullptr)
   {
      struct dirent *d;
      while((d = readdir(dir)) != nullptr)
      {
         if (!strncmp(d->d_name, "intel-rapl:", 11))
         {
            RegisterPowerZone(path, d->d_name, zone);
         }
      }
      closedir(dir);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open Intel RAPL sysfs directory %s (%s)"), path, _tcserror(errno));
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Power zone \"%hs\" registered"), zone->name);
}

/**
 * Scan sysfs for Intel RAPL power zones. We assume that zones will not change after bot.
 */
void ScanRAPLPowerZones()
{
   DIR *dir = opendir("/sys/devices/virtual/powercap/intel-rapl");
   if (dir == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open Intel RAPL sysfs root (%s)"), _tcserror(errno));
      return;
   }

   struct dirent *d;
   while((d = readdir(dir)) != nullptr)
   {
      if (!strncmp(d->d_name, "intel-rapl:", 11))
      {
         RegisterPowerZone("/sys/devices/virtual/powercap/intel-rapl", d->d_name, nullptr);
      }
   }

   closedir(dir);
}

/**
 * Handler for System.PowerZones list
 */
LONG H_PowerZoneList(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(PowerZone *zone = s_powerZones; zone != nullptr; zone = zone->next)
      value->addUTF8String(zone->name);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.EnergyConsumption.PowerZone(*)
 */
LONG H_ZoneEnergyConsumption(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (s_powerZones == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   char zoneName[256];
   AgentGetParameterArgA(metric, 1, zoneName, 256);
   for(PowerZone *zone = s_powerZones; zone != nullptr; zone = zone->next)
   {
      if (!stricmp(zone->name, zoneName))
      {
         uint64_t v;
         if (!ReadUInt64FromFileA(zone->energyDataPath, &v))
            return SYSINFO_RC_ERROR;
         ret_uint64(value, v);
         return SYSINFO_RC_SUCCESS;
      }
   }

   return SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Handler for System.EnergyConsumption.Total
 */
LONG H_TotalEnergyConsumption(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (s_powerZones == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   uint64_t total = 0;
   for(PowerZone *zone = s_powerZones; zone != nullptr; zone = zone->next)
   {
      if (zone->parent == nullptr)
      {
         uint64_t v;
         if (ReadUInt64FromFileA(zone->energyDataPath, &v))
            total += v;
      }
   }
   ret_uint64(value, total);
   return SYSINFO_RC_SUCCESS;
}
