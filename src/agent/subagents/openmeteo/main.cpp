/*
** NetXMS Open-Meteo weather subagent
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

#include "openmeteo.h"
#include <netxms-version.h>

/**
 * Named location alias: a friendly name mapped to coordinates. Read-only after
 * initialization, so no locking is needed for lookup.
 */
struct LocationAlias
{
   TCHAR name[MAX_LOC_NAME];
   double latitude;
   double longitude;
};

/**
 * Global state
 */
static StructArray<LocationAlias> s_aliases(16, 16);
static ObjectArray<MeteoLocation> s_locations(16, 16, Ownership::True);
static Mutex s_locationsLock;
static OpenMeteoClient *s_client = nullptr;
static uint32_t s_pollInterval = 900;
static int s_forecastDays = 2;
static bool s_ensembleEnabled = false;
static char s_ensembleModel[64] = "icon_seamless";
static Condition s_shutdownCondition(true);
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;

/**
 * Find a cached location by canonical key. Caller must hold s_locationsLock.
 */
static MeteoLocation *FindLocationByKey(const char *key)
{
   for(int i = 0; i < s_locations.size(); i++)
      if (!strcmp(s_locations.get(i)->getKey(), key))
         return s_locations.get(i);
   return nullptr;
}

/**
 * Find an existing cached location by coordinates or create one, so a named
 * location and an equal raw pair converge on a single polled entry.
 */
static MeteoLocation *FindOrCreateLocation(const TCHAR *displayName, double latitude, double longitude)
{
   char key[MAX_LOC_KEY];
   FormatLocationKey(latitude, longitude, key);

   LockGuard lockGuard(s_locationsLock);
   MeteoLocation *loc = FindLocationByKey(key);
   if (loc == nullptr)
   {
      loc = new MeteoLocation(displayName, latitude, longitude);
      s_locations.add(loc);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Location %s registered (%hs)"), displayName, key);
   }
   return loc;
}

/**
 * Find a configured alias by name (case-insensitive).
 */
static const LocationAlias *FindAlias(const TCHAR *name)
{
   for(int i = 0; i < s_aliases.size(); i++)
      if (!_tcsicmp(s_aliases.get(i)->name, name))
         return s_aliases.get(i);
   return nullptr;
}

/**
 * Resolve a metric instance to a location. Accepts either a raw "lat,lon" pair
 * (registered on demand) or a configured location name. Returns nullptr for an
 * unknown name or malformed coordinates.
 */
static MeteoLocation *ResolveLocation(const TCHAR *instance)
{
   double lat, lon;
   if (ParseLatLon(instance, &lat, &lon))
      return FindOrCreateLocation(instance, lat, lon);

   const LocationAlias *alias = FindAlias(instance);
   if (alias == nullptr)
      return nullptr;
   return FindOrCreateLocation(alias->name, alias->latitude, alias->longitude);
}

/**
 * Load configured named locations. Each value has the form "name: lat, lon".
 */
static void LoadLocations(Config *config)
{
   ConfigEntry *entry = config->getEntry(_T("/OpenMeteo/Location"));
   if (entry == nullptr)
      return;

   for(int i = 0; i < entry->getValueCount(); i++)
   {
      TCHAR value[256];
      _tcslcpy(value, entry->getValue(i), 256);

      TCHAR *colon = _tcschr(value, _T(':'));
      if (colon == nullptr)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Ignoring malformed location \"%s\" (expected \"name: lat, lon\")"), value);
         continue;
      }
      *colon = 0;

      LocationAlias alias;
      _tcslcpy(alias.name, value, MAX_LOC_NAME);
      Trim(alias.name);
      if ((alias.name[0] == 0) || !ParseLatLon(colon + 1, &alias.latitude, &alias.longitude))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Ignoring location with invalid coordinates: %s"), entry->getValue(i));
         continue;
      }

      s_aliases.add(&alias);
      FindOrCreateLocation(alias.name, alias.latitude, alias.longitude);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Location %s configured (%f, %f)"), alias.name, alias.latitude, alias.longitude);
   }
}

/**
 * Poller thread
 */
static void PollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Poller thread started (interval=%u s)"), s_pollInterval);
   do
   {
      int64_t startTime = GetCurrentTimeMs();

      // Snapshot the registry: locations are never removed, so borrowed pointers
      // stay valid while we poll without holding the registry lock.
      ObjectArray<MeteoLocation> snapshot(s_locations.size(), 16, Ownership::False);
      s_locationsLock.lock();
      for(int i = 0; i < s_locations.size(); i++)
         snapshot.add(s_locations.get(i));
      s_locationsLock.unlock();

      for(int i = 0; i < snapshot.size(); i++)
      {
         snapshot.get(i)->poll(*s_client, s_forecastDays, s_ensembleEnabled, s_ensembleModel);
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
 * Scalar metric handler for current conditions. arg[0] selects the field.
 */
static LONG H_Current(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!AgentGetParameterArg(cmd, 1, instance, MAX_LOC_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   Trim(instance);
   if (_istdigit(instance[0]) && (_tcschr(instance, _T(',')) == nullptr))
   {
      TCHAR part2[64];
      if (AgentGetParameterArg(cmd, 2, part2, 64))
      {
         _tcslcat(instance, _T(","), MAX_LOC_NAME);
         _tcslcat(instance, part2, MAX_LOC_NAME);
      }
   }

   MeteoLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   WeatherField field;
   switch(*arg)
   {
      case _T('T'): field = WeatherField::TEMPERATURE; break;
      case _T('C'): field = WeatherField::CLOUD_COVER; break;
      case _T('S'): field = WeatherField::SHORTWAVE_RADIATION; break;
      case _T('D'): field = WeatherField::DIRECT_RADIATION; break;
      case _T('W'): field = WeatherField::WIND_SPEED; break;
      case _T('H'): field = WeatherField::RELATIVE_HUMIDITY; break;
      default: return SYSINFO_RC_UNSUPPORTED;
   }

   double v;
   if (!loc->getCurrent(field, &v))
      return SYSINFO_RC_ERROR;
   ret_double(value, v);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Data age metric handler.
 */
static LONG H_DataAge(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!AgentGetParameterArg(cmd, 1, instance, MAX_LOC_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   Trim(instance);
   if (_istdigit(instance[0]) && (_tcschr(instance, _T(',')) == nullptr))
   {
      TCHAR part2[64];
      if (AgentGetParameterArg(cmd, 2, part2, 64))
      {
         _tcslcat(instance, _T(","), MAX_LOC_NAME);
         _tcslcat(instance, part2, MAX_LOC_NAME);
      }       
   }

   MeteoLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   int64_t seconds;
   if (!loc->getDataAge(&seconds))
      return SYSINFO_RC_ERROR;
   ret_int64(value, seconds);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Table handlers
 */
static LONG H_ForecastTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!AgentGetParameterArg(cmd, 1, instance, MAX_LOC_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   Trim(instance);
   if (_istdigit(instance[0]) && (_tcschr(instance, _T(',')) == nullptr))
   {
      TCHAR part2[64];
      if (AgentGetParameterArg(cmd, 2, part2, 64))
      {
         _tcslcat(instance, _T(","), MAX_LOC_NAME);
         _tcslcat(instance, part2, MAX_LOC_NAME);
      }       
   }

   MeteoLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return loc->fillForecastTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static LONG H_EnsembleTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!AgentGetParameterArg(cmd, 1, instance, MAX_LOC_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   Trim(instance);
   if (_istdigit(instance[0]) && (_tcschr(instance, _T(',')) == nullptr))
   {
      TCHAR part2[64];
      if (AgentGetParameterArg(cmd, 2, part2, 64))
      {
         _tcslcat(instance, _T(","), MAX_LOC_NAME);
         _tcslcat(instance, part2, MAX_LOC_NAME);
      }       
   }

   MeteoLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return loc->fillEnsembleTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * List handler: configured location names.
 */
static LONG H_LocationsList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_aliases.size(); i++)
      value->add(s_aliases.get(i)->name);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   s_pollInterval = config->getValueAsUInt(_T("/OpenMeteo/PollInterval"), 900);
   if (s_pollInterval < 60)
      s_pollInterval = 60;
   s_forecastDays = config->getValueAsInt(_T("/OpenMeteo/ForecastDays"), 2);
   if (s_forecastDays < 1)
      s_forecastDays = 1;
   else if (s_forecastDays > 16)
      s_forecastDays = 16;
   s_ensembleEnabled = config->getValueAsBoolean(_T("/OpenMeteo/EnableEnsemble"), false);
#ifdef UNICODE
   wchar_to_utf8(config->getValue(_T("/OpenMeteo/EnsembleModel"), _T("icon_seamless")), -1, s_ensembleModel, sizeof(s_ensembleModel));
   s_ensembleModel[sizeof(s_ensembleModel) - 1] = 0;
#else
   strlcpy(s_ensembleModel, config->getValue(_T("/OpenMeteo/EnsembleModel"), _T("icon_seamless")), sizeof(s_ensembleModel));
#endif
   uint32_t timeout = config->getValueAsUInt(_T("/OpenMeteo/RequestTimeout"), 30) * 1000;

   LoadLocations(config);

   s_client = new OpenMeteoClient(timeout);
   s_pollerThread = ThreadCreateEx(PollerThread);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Open-Meteo subagent initialized (%d configured locations, ensemble %s)"),
      s_aliases.size(), s_ensembleEnabled ? _T("enabled") : _T("disabled"));
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
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Open-Meteo subagent shutdown completed"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("OpenMeteo.CloudCover(*)"), H_Current, _T("C"), DCI_DT_FLOAT, _T("Open-Meteo: cloud cover for {instance} (%)") },
   { _T("OpenMeteo.DataAge(*)"), H_DataAge, nullptr, DCI_DT_INT64, _T("Open-Meteo: age of current observation for {instance} (seconds)") },
   { _T("OpenMeteo.DirectRadiation(*)"), H_Current, _T("D"), DCI_DT_FLOAT, _T("Open-Meteo: direct radiation for {instance} (W/m2)") },
   { _T("OpenMeteo.RelativeHumidity(*)"), H_Current, _T("H"), DCI_DT_FLOAT, _T("Open-Meteo: relative humidity for {instance} (%)") },
   { _T("OpenMeteo.ShortwaveRadiation(*)"), H_Current, _T("S"), DCI_DT_FLOAT, _T("Open-Meteo: shortwave radiation for {instance} (W/m2)") },
   { _T("OpenMeteo.Temperature(*)"), H_Current, _T("T"), DCI_DT_FLOAT, _T("Open-Meteo: temperature for {instance} (C)") },
   { _T("OpenMeteo.WindSpeed(*)"), H_Current, _T("W"), DCI_DT_FLOAT, _T("Open-Meteo: wind speed for {instance} (km/h)") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("OpenMeteo.Locations"), H_LocationsList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("OpenMeteo.Forecast(*)"), H_ForecastTable, nullptr, _T("TIME"), _T("Open-Meteo: hourly weather/solar forecast curve") },
   { _T("OpenMeteo.ForecastEnsemble(*)"), H_EnsembleTable, nullptr, _T("TIME"), _T("Open-Meteo: ensemble forecast spread (solar/temperature)") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("OPENMETEO"), NETXMS_VERSION_STRING,
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
DECLARE_SUBAGENT_ENTRY_POINT(OPENMETEO)
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
