/*
** NetXMS ENTSO-E Transparency Platform subagent
** Copyright (C) 2026 Raden Solutions
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
** File: main.cpp
**
**/

#include "entsoe.h"
#include <netxms-version.h>

/**
 * Global state
 */
static ObjectArray<EntsoeZone> s_zones(16, 16, Ownership::True);
static EmissionFactorSet s_factors;
static EntsoeClient *s_client = nullptr;
static uint32_t s_pollInterval = 900;
static uint32_t s_lookback = 10800;
static Condition s_shutdownCondition(true);
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;

/**
 * Convert TCHAR to ASCII (for codes/names known to be ASCII).
 */
static inline void TcharToAscii(const TCHAR *src, char *dst, size_t size)
{
#ifdef UNICODE
   size_t i = 0;
   for(; (src[i] != 0) && (i < size - 1); i++)
      dst[i] = static_cast<char>(src[i]);
   dst[i] = 0;
#else
   strlcpy(dst, src, size);
#endif
}

/**
 * Convert ASCII to TCHAR.
 */
static inline void AsciiToTchar(const char *src, TCHAR *dst, size_t size)
{
#ifdef UNICODE
   size_t i = 0;
   for(; (src[i] != 0) && (i < size - 1); i++)
      dst[i] = static_cast<TCHAR>(static_cast<unsigned char>(src[i]));
   dst[i] = 0;
#else
   strlcpy(dst, src, size);
#endif
}

/**
 * Normalize a zone name for matching: uppercase, strip separators.
 */
static void NormalizeName(const TCHAR *in, TCHAR *out, size_t size)
{
   size_t j = 0;
   for(size_t i = 0; (in[i] != 0) && (j < size - 1); i++)
   {
      TCHAR c = in[i];
      if (_istalnum(c))
         out[j++] = _totupper(c);
   }
   out[j] = 0;
}

/**
 * Find configured zone by name.
 */
static EntsoeZone *FindZoneByName(const TCHAR *name)
{
   TCHAR norm[MAX_ZONE_NAME];
   NormalizeName(name, norm, MAX_ZONE_NAME);
   for(int i = 0; i < s_zones.size(); i++)
   {
      TCHAR n2[MAX_ZONE_NAME];
      NormalizeName(s_zones.get(i)->getName(), n2, MAX_ZONE_NAME);
      if (!_tcscmp(norm, n2))
         return s_zones.get(i);
   }
   return nullptr;
}

/**
 * Remove any existing zone with the given name (for explicit-block override).
 */
static void RemoveZoneByName(const TCHAR *name)
{
   EntsoeZone *z = FindZoneByName(name);
   if (z != nullptr)
      s_zones.remove(z);   // Ownership::True -> destroys
}

/**
 * Create a zone from the catalog, wiring up borders from catalog adjacency.
 */
static EntsoeZone *CreateZoneFromCatalog(const char *nameMB)
{
   const CatalogZone *cz = LookupCatalogZone(nameMB);
   if (cz == nullptr)
      return nullptr;

   TCHAR nameT[MAX_ZONE_NAME];
   AsciiToTchar(cz->name, nameT, MAX_ZONE_NAME);
   EntsoeZone *zone = new EntsoeZone(nameT, cz->eic);

   for(int i = 0; i < cz->neighbours.size(); i++)
   {
      char nbMB[MAX_ZONE_NAME];
      TcharToAscii(cz->neighbours.get(i), nbMB, MAX_ZONE_NAME);
      const char *nbEic = LookupZoneEic(nbMB);
      if (nbEic != nullptr)
         zone->addBorder(nbMB, nbEic);
   }
   return zone;
}

/**
 * Load zones listed in the Zones= directive (resolved against the catalog).
 */
static void LoadZonesFromList(Config *config)
{
   const TCHAR *value = config->getValue(_T("/ENTSOE/Zones"), nullptr);
   if ((value == nullptr) || (*value == 0))
      return;

   StringList names = String(value).split(_T(","), true);
   for(int i = 0; i < names.size(); i++)
   {
      TCHAR nameT[MAX_ZONE_NAME];
      _tcslcpy(nameT, names.get(i), MAX_ZONE_NAME);
      Trim(nameT);
      if (*nameT == 0)
         continue;

      char nameMB[MAX_ZONE_NAME];
      TcharToAscii(nameT, nameMB, MAX_ZONE_NAME);
      EntsoeZone *zone = CreateZoneFromCatalog(nameMB);
      if (zone != nullptr)
      {
         s_zones.add(zone);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Zone %s added from catalog (EIC %hs)"), zone->getName(), zone->getEic());
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Zone %s not found in catalog; add an [ENTSOE/Zone/%s] block with an explicit EIC"), nameT, nameT);
      }
   }
}

/**
 * Load explicit [ENTSOE/Zone/NAME] blocks (override/extend the catalog).
 */
static void LoadZonesFromBlocks(Config *config)
{
   unique_ptr<ObjectArray<ConfigEntry>> blocks = config->getSubEntries(_T("/ENTSOE/Zone"), _T("*"));
   if (blocks == nullptr)
      return;

   for(int i = 0; i < blocks->size(); i++)
   {
      ConfigEntry *block = blocks->get(i);
      const TCHAR *name = block->getName();
      const TCHAR *eic = block->getSubEntryValue(_T("EIC"), 0, nullptr);
      if ((eic == nullptr) || (*eic == 0))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Zone block %s has no EIC; ignored"), name);
         continue;
      }

      RemoveZoneByName(name);

      char eicMB[MAX_EIC_LEN];
      TcharToAscii(eic, eicMB, MAX_EIC_LEN);
      EntsoeZone *zone = new EntsoeZone(name, eicMB);

      ConfigEntry *borders = block->findEntry(_T("Border"));
      if (borders != nullptr)
      {
         for(int j = 0; j < borders->getValueCount(); j++)
         {
            // "NAME:EIC" or "NAME" (EIC looked up from catalog)
            TCHAR spec[128];
            _tcslcpy(spec, borders->getValue(j), 128);
            TCHAR *colon = _tcschr(spec, _T(':'));
            char nbMB[MAX_ZONE_NAME], nbEicMB[MAX_EIC_LEN];
            if (colon != nullptr)
            {
               *colon = 0;
               TcharToAscii(spec, nbMB, MAX_ZONE_NAME);
               TcharToAscii(colon + 1, nbEicMB, MAX_EIC_LEN);
               zone->addBorder(nbMB, nbEicMB);
            }
            else
            {
               TcharToAscii(spec, nbMB, MAX_ZONE_NAME);
               const char *nbEic = LookupZoneEic(nbMB);
               if (nbEic != nullptr)
                  zone->addBorder(nbMB, nbEic);
            }
         }
      }
      s_zones.add(zone);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Zone %s added from configuration block (EIC %hs)"), name, eicMB);
   }
}

/**
 * Poller thread
 */
static void PollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Poller thread started (interval=%u s, %d zones)"), s_pollInterval, s_zones.size());
   do
   {
      int64_t startTime = GetCurrentTimeMs();
      for(int i = 0; i < s_zones.size(); i++)
      {
         s_zones.get(i)->poll(*s_client, s_factors, s_lookback);
         if (s_shutdownCondition.wait(0))
            break;
      }
      int64_t elapsed = GetCurrentTimeMs() - startTime;
      int64_t intervalMs = static_cast<int64_t>(s_pollInterval) * 1000;
      uint32_t sleepTime = (elapsed >= intervalMs) ? 1000 : static_cast<uint32_t>(intervalMs - elapsed);
      if (s_shutdownCondition.wait(sleepTime))
         break;
   } while(true);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Poller thread stopped"));
}

/**
 * Scalar metric handlers
 */
static LONG H_CarbonIntensity(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   double v;
   if (!zone->getCarbonIntensity(&v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

static LONG H_Share(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   double v;
   if (!zone->getShare(*arg == _T('L'), &v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

static LONG H_TotalGeneration(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   double v;
   if (!zone->getTotalGeneration(&v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

static LONG H_GenerationByType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME], psrType[MAX_PSR_TYPE];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME) ||
       !AgentGetParameterArg(cmd, 2, psrType, MAX_PSR_TYPE))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   char psrTypeMB[MAX_PSR_TYPE];
   TcharToAscii(psrType, psrTypeMB, MAX_PSR_TYPE);
   double v;
   if (!zone->getGenerationByType(psrTypeMB, &v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

static LONG H_NetImport(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   double v;
   if (!zone->getNetImport(&v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

static LONG H_DataAge(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   int64_t seconds;
   if (!zone->getDataAge(&seconds))
      return SYSINFO_RC_ERROR;
   ret_int64(value, seconds);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Table handlers
 */
static LONG H_GenerationTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return zone->fillGenerationTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static LONG H_FlowsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return zone->fillFlowsTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static LONG H_ForecastTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return zone->fillForecastTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * List handlers
 */
static LONG H_ZonesList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_zones.size(); i++)
      value->add(s_zones.get(i)->getName());
   return SYSINFO_RC_SUCCESS;
}

static LONG H_GenerationTypesList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR zoneName[MAX_ZONE_NAME];
   if (!AgentGetParameterArg(cmd, 1, zoneName, MAX_ZONE_NAME))
      return SYSINFO_RC_UNSUPPORTED;
   EntsoeZone *zone = FindZoneByName(zoneName);
   if (zone == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   zone->fillGenerationTypes(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   InitZoneCatalog();
   const TCHAR *catalogFile = config->getValue(_T("/ENTSOE/ZoneCatalog"), nullptr);
   if ((catalogFile != nullptr) && (*catalogFile != 0))
      LoadZoneCatalogFile(catalogFile);

   s_factors.loadDefaults();
   s_factors.setDefaultFactor(_tcstod(config->getValue(_T("/ENTSOE/DefaultFactor"), _T("500")), nullptr));
   const TCHAR *factorFile = config->getValue(_T("/ENTSOE/EmissionFactors"), nullptr);
   if ((factorFile != nullptr) && (*factorFile != 0))
      s_factors.loadFromFile(factorFile);

   s_pollInterval = config->getValueAsUInt(_T("/ENTSOE/PollInterval"), 900);
   if (s_pollInterval < 60)
      s_pollInterval = 60;
   s_lookback = config->getValueAsUInt(_T("/ENTSOE/Lookback"), 10800);
   uint32_t timeout = config->getValueAsUInt(_T("/ENTSOE/RequestTimeout"), 30) * 1000;

   LoadZonesFromList(config);
   LoadZonesFromBlocks(config);

   const TCHAR *token = config->getValue(_T("/ENTSOE/Token"), _T(""));
   if (*token == 0)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No API token configured; subagent loaded but inactive"));
      return true;
   }
   if (s_zones.isEmpty())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No zones configured; subagent loaded but inactive"));
      return true;
   }

   char tokenMB[128];
   TcharToAscii(token, tokenMB, sizeof(tokenMB));
   s_client = new EntsoeClient(tokenMB, timeout);

   s_pollerThread = ThreadCreateEx(PollerThread);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("ENTSO-E subagent initialized (%d zones)"), s_zones.size());
   return true;
}

/**
 * Subagent shutdown
 */
static void SubagentShutdown()
{
   s_shutdownCondition.set();
   if (s_pollerThread != INVALID_THREAD_HANDLE)
      ThreadJoin(s_pollerThread);
   delete s_client;
   s_client = nullptr;
   nxlog_debug_tag(DEBUG_TAG, 2, _T("ENTSO-E subagent shutdown completed"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("ENTSOE.CarbonIntensity(*)"), H_CarbonIntensity, nullptr, DCI_DT_FLOAT, _T("ENTSO-E: carbon intensity for {instance} (gCO2/kWh)") },
   { _T("ENTSOE.DataAge(*)"), H_DataAge, nullptr, DCI_DT_INT64, _T("ENTSO-E: age of served generation interval for {instance} (seconds)") },
   { _T("ENTSOE.Generation(*)"), H_GenerationByType, nullptr, DCI_DT_FLOAT, _T("ENTSO-E: generation for {instance} (MW)") },
   { _T("ENTSOE.LowCarbonShare(*)"), H_Share, _T("L"), DCI_DT_FLOAT, _T("ENTSO-E: low-carbon share for {instance} (%)") },
   { _T("ENTSOE.NetImport(*)"), H_NetImport, nullptr, DCI_DT_FLOAT, _T("ENTSO-E: net cross-border import for {instance} (MW)") },
   { _T("ENTSOE.RenewableShare(*)"), H_Share, _T("R"), DCI_DT_FLOAT, _T("ENTSO-E: renewable share for {instance} (%)") },
   { _T("ENTSOE.TotalGeneration(*)"), H_TotalGeneration, nullptr, DCI_DT_FLOAT, _T("ENTSO-E: total generation for {instance} (MW)") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("ENTSOE.GenerationTypes(*)"), H_GenerationTypesList, nullptr },
   { _T("ENTSOE.Zones"), H_ZonesList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("ENTSOE.CrossBorderFlows(*)"), H_FlowsTable, nullptr, _T("NEIGHBOUR"), _T("ENTSO-E: cross-border flows") },
   { _T("ENTSOE.Forecast(*)"), H_ForecastTable, nullptr, _T("TIME"), _T("ENTSO-E: wind/solar/load forecast curve") },
   { _T("ENTSOE.Generation(*)"), H_GenerationTable, nullptr, _T("CODE"), _T("ENTSO-E: generation per production type") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("ENTSOE"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(ENTSOE)
{
   *ppInfo = &s_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
