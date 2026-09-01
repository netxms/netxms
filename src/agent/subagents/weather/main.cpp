/*
** NetXMS weather subagent
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

#include "weather.h"
#include <netxms-version.h>

/**
 * Named location alias: a friendly name mapped to coordinates and a provider.
 * Read-only after initialization, so no locking is needed for lookup.
 */
struct LocationAlias
{
   TCHAR name[MAX_LOC_NAME];
   double latitude;
   double longitude;
   const WeatherProvider *provider;
};

/**
 * Global state
 */
static StructArray<LocationAlias> s_aliases(16, 16);
static ObjectArray<WeatherLocation> s_locations(16, 16, Ownership::True);
static Mutex s_locationsLock;
static ObjectArray<WeatherProvider> s_providers(4, 4, Ownership::True);
static const WeatherProvider *s_defaultProvider = nullptr;
static HttpClient *s_client = nullptr;
static uint32_t s_pollInterval = 900;
static int s_forecastDays = 2;
static bool s_ensembleEnabled = false;
static Condition s_shutdownCondition(true);
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;

/**
 * Find a registered provider adapter by name (case-insensitive).
 */
static const WeatherProvider *FindProvider(const TCHAR *name)
{
   char nameMB[MAX_PROVIDER_NAME];
   size_t bytes = tchar_to_utf8(name, -1, nameMB, MAX_PROVIDER_NAME - 1);
   nameMB[bytes] = 0;
   for(int i = 0; i < s_providers.size(); i++)
      if (!stricmp(s_providers.get(i)->getName(), nameMB))
         return s_providers.get(i);
   return nullptr;
}

/**
 * Find a cached location by canonical key. Caller must hold s_locationsLock.
 */
static WeatherLocation *FindLocationByKey(const char *key)
{
   for(int i = 0; i < s_locations.size(); i++)
      if (!strcmp(s_locations.get(i)->getKey(), key))
         return s_locations.get(i);
   return nullptr;
}

/**
 * Find an existing cached location by provider and coordinates or create one, so
 * a named location and an equal raw pair converge on a single polled entry.
 */
static WeatherLocation *FindOrCreateLocation(const TCHAR *displayName, double latitude, double longitude, const WeatherProvider *provider)
{
   char key[MAX_LOC_KEY];
   FormatLocationKey(provider->getName(), latitude, longitude, key);

   LockGuard lockGuard(s_locationsLock);
   WeatherLocation *loc = FindLocationByKey(key);
   if (loc == nullptr)
   {
      loc = new WeatherLocation(displayName, latitude, longitude, provider);
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
 * (registered on demand with the default provider) or a configured location
 * name. Returns nullptr for an unknown name or malformed coordinates.
 */
static WeatherLocation *ResolveLocation(const TCHAR *instance)
{
   double lat, lon;
   if (ParseLatLon(instance, &lat, &lon))
      return FindOrCreateLocation(instance, lat, lon, s_defaultProvider);

   const LocationAlias *alias = FindAlias(instance);
   if (alias == nullptr)
      return nullptr;
   return FindOrCreateLocation(alias->name, alias->latitude, alias->longitude, alias->provider);
}

/**
 * Register a configured location under the given name, coordinates and provider.
 * A later definition of the same name replaces the earlier one.
 */
static void AddAlias(const TCHAR *name, double latitude, double longitude, const WeatherProvider *provider)
{
   for(int i = 0; i < s_aliases.size(); i++)
   {
      if (!_tcsicmp(s_aliases.get(i)->name, name))
      {
         s_aliases.remove(i);
         break;
      }
   }

   LocationAlias alias;
   _tcslcpy(alias.name, name, MAX_LOC_NAME);
   alias.latitude = latitude;
   alias.longitude = longitude;
   alias.provider = provider;
   s_aliases.add(&alias);

   FindOrCreateLocation(alias.name, latitude, longitude, provider);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Location %s configured (%f, %f, provider %hs)"),
      alias.name, latitude, longitude, provider->getName());
}

/**
 * Load locations from the Location directive. Each value has the form
 * "name: lat, lon" and uses the default provider.
 */
static void LoadLocationsFromList(Config *config)
{
   ConfigEntry *entry = config->getEntry(_T("/Weather/Location"));
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

      TCHAR name[MAX_LOC_NAME];
      _tcslcpy(name, value, MAX_LOC_NAME);
      Trim(name);

      double latitude, longitude;
      if ((name[0] == 0) || !ParseLatLon(colon + 1, &latitude, &longitude))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Ignoring location with invalid coordinates: %s"), entry->getValue(i));
         continue;
      }

      AddAlias(name, latitude, longitude, s_defaultProvider);
   }
}

/**
 * Load explicit [Weather/Location/NAME] blocks. Unlike the Location directive
 * these can select a provider per location, which matters because provider
 * capabilities differ (only Open-Meteo reports solar irradiance).
 */
static void LoadLocationsFromBlocks(Config *config)
{
   unique_ptr<ObjectArray<ConfigEntry>> blocks = config->getSubEntries(_T("/Weather/Location"), _T("*"));
   if (blocks == nullptr)
      return;

   for(int i = 0; i < blocks->size(); i++)
   {
      ConfigEntry *block = blocks->get(i);
      const TCHAR *name = block->getName();

      const TCHAR *latitude = block->getSubEntryValue(_T("Latitude"), 0, nullptr);
      const TCHAR *longitude = block->getSubEntryValue(_T("Longitude"), 0, nullptr);
      if ((latitude == nullptr) || (longitude == nullptr))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Location block %s has no Latitude/Longitude; ignored"), name);
         continue;
      }

      TCHAR coordinates[128];
      _sntprintf(coordinates, 128, _T("%s,%s"), latitude, longitude);
      double lat, lon;
      if (!ParseLatLon(coordinates, &lat, &lon))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Location block %s has invalid coordinates (%s, %s); ignored"), name, latitude, longitude);
         continue;
      }

      const WeatherProvider *provider = s_defaultProvider;
      const TCHAR *providerName = block->getSubEntryValue(_T("Provider"), 0, nullptr);
      if (providerName != nullptr)
      {
         provider = FindProvider(providerName);
         if (provider == nullptr)
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unknown weather provider \"%s\" for location %s; ignored"), providerName, name);
            continue;
         }
      }

      AddAlias(name, lat, lon, provider);
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
      ObjectArray<WeatherLocation> snapshot(s_locations.size(), 16, Ownership::False);
      s_locationsLock.lock();
      for(int i = 0; i < s_locations.size(); i++)
         snapshot.add(s_locations.get(i));
      s_locationsLock.unlock();

      for(int i = 0; i < snapshot.size(); i++)
      {
         snapshot.get(i)->poll(*s_client, s_forecastDays, s_ensembleEnabled);
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
 * Read the location instance from a metric request. A raw coordinate pair may
 * arrive split across two arguments because the comma also separates arguments.
 */
static bool GetLocationInstance(const TCHAR *cmd, TCHAR *instance)
{
   if (!AgentGetParameterArg(cmd, 1, instance, MAX_LOC_NAME))
      return false;

   Trim(instance);
   if ((_istdigit(instance[0]) || (instance[0] == _T('-')) || (instance[0] == _T('+'))) && (_tcschr(instance, _T(',')) == nullptr))
   {
      TCHAR part2[64];
      if (AgentGetParameterArg(cmd, 2, part2, 64))
      {
         _tcslcat(instance, _T(","), MAX_LOC_NAME);
         _tcslcat(instance, part2, MAX_LOC_NAME);
      }
   }
   return true;
}

/**
 * Scalar metric handler for current conditions. arg[0] selects the field.
 */
static LONG H_Current(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!GetLocationInstance(cmd, instance))
      return SYSINFO_RC_UNSUPPORTED;

   WeatherLocation *loc = ResolveLocation(instance);
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
      case _T('P'): field = WeatherField::PRECIPITATION; break;
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
   if (!GetLocationInstance(cmd, instance))
      return SYSINFO_RC_UNSUPPORTED;

   WeatherLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   int64_t seconds;
   if (!loc->getDataAge(&seconds))
      return SYSINFO_RC_ERROR;
   ret_int64(value, seconds);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Provider name metric handler.
 */
static LONG H_Provider(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!GetLocationInstance(cmd, instance))
      return SYSINFO_RC_UNSUPPORTED;

   WeatherLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   ret_mbstring(value, loc->getProvider()->getName());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Table handlers
 */
static LONG H_ForecastTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!GetLocationInstance(cmd, instance))
      return SYSINFO_RC_UNSUPPORTED;

   WeatherLocation *loc = ResolveLocation(instance);
   if (loc == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return loc->fillForecastTable(value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static LONG H_EnsembleTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR instance[MAX_LOC_NAME];
   if (!GetLocationInstance(cmd, instance))
      return SYSINFO_RC_UNSUPPORTED;

   WeatherLocation *loc = ResolveLocation(instance);
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
   s_pollInterval = config->getValueAsUInt(_T("/Weather/PollInterval"), 900);
   if (s_pollInterval < 60)
      s_pollInterval = 60;
   s_forecastDays = config->getValueAsInt(_T("/Weather/ForecastDays"), 2);
   if (s_forecastDays < 1)
      s_forecastDays = 1;
   else if (s_forecastDays > 16)
      s_forecastDays = 16;
   s_ensembleEnabled = config->getValueAsBoolean(_T("/Weather/EnableEnsemble"), false);
   uint32_t timeout = config->getValueAsUInt(_T("/Weather/RequestTimeout"), 30) * 1000;

   char apiKey[MAX_API_KEY], ensembleModel[64];
   size_t bytes = tchar_to_utf8(config->getValue(_T("/Weather/ApiKey"), _T("")), -1, apiKey, sizeof(apiKey) - 1);
   apiKey[bytes] = 0;
   bytes = tchar_to_utf8(config->getValue(_T("/Weather/EnsembleModel"), _T("icon_seamless")), -1, ensembleModel, sizeof(ensembleModel) - 1);
   ensembleModel[bytes] = 0;

   s_providers.add(new OpenMeteoProvider(apiKey, ensembleModel));
   s_providers.add(new MetNoProvider());

   const TCHAR *defaultProviderName = config->getValue(_T("/Weather/Provider"), _T("openmeteo"));
   s_defaultProvider = FindProvider(defaultProviderName);
   if (s_defaultProvider == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unknown weather provider \"%s\" configured as default"), defaultProviderName);
      s_providers.clear();
      return false;
   }

   // MET Norway blocks generic user agents, so an operator-supplied identity
   // with contact information is required before any location can use it.
   char userAgent[MAX_USER_AGENT];
   const TCHAR *configuredUserAgent = config->getValue(_T("/Weather/UserAgent"), nullptr);
   if (configuredUserAgent != nullptr)
   {
      bytes = tchar_to_utf8(configuredUserAgent, -1, userAgent, sizeof(userAgent) - 1);
      userAgent[bytes] = 0;
   }
   else
   {
      strlcpy(userAgent, "NetXMS Agent/" NETXMS_VERSION_STRING_A, sizeof(userAgent));
   }

   LoadLocationsFromList(config);
   LoadLocationsFromBlocks(config);

   if (configuredUserAgent == nullptr)
   {
      for(int i = 0; i < s_aliases.size(); i++)
      {
         if (!strcmp(s_aliases.get(i)->provider->getName(), "metno"))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
               _T("MET Norway requires an identifying User-Agent with contact information; set UserAgent in the [Weather] section or requests will be rejected"));
            break;
         }
      }
   }

   s_client = new HttpClient(timeout, userAgent);
   s_pollerThread = ThreadCreateEx(PollerThread);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Weather subagent initialized (%d configured locations, default provider %hs, ensemble %s)"),
      s_aliases.size(), s_defaultProvider->getName(), s_ensembleEnabled ? _T("enabled") : _T("disabled"));
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
   s_locations.clear();
   s_aliases.clear();
   s_defaultProvider = nullptr;
   s_providers.clear();
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Weather subagent shutdown completed"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Weather.CloudCover(*)"), H_Current, _T("C"), DCI_DT_FLOAT, _T("Weather: cloud cover for {instance} (%)") },
   { _T("Weather.DataAge(*)"), H_DataAge, nullptr, DCI_DT_INT64, _T("Weather: age of current observation for {instance} (seconds)") },
   { _T("Weather.DirectRadiation(*)"), H_Current, _T("D"), DCI_DT_FLOAT, _T("Weather: direct radiation for {instance} (W/m2)") },
   { _T("Weather.Precipitation(*)"), H_Current, _T("P"), DCI_DT_FLOAT, _T("Weather: precipitation for {instance} (mm)") },
   { _T("Weather.Provider(*)"), H_Provider, nullptr, DCI_DT_STRING, _T("Weather: data provider for {instance}") },
   { _T("Weather.RelativeHumidity(*)"), H_Current, _T("H"), DCI_DT_FLOAT, _T("Weather: relative humidity for {instance} (%)") },
   { _T("Weather.ShortwaveRadiation(*)"), H_Current, _T("S"), DCI_DT_FLOAT, _T("Weather: shortwave radiation for {instance} (W/m2)") },
   { _T("Weather.Temperature(*)"), H_Current, _T("T"), DCI_DT_FLOAT, _T("Weather: temperature for {instance} (C)") },
   { _T("Weather.WindSpeed(*)"), H_Current, _T("W"), DCI_DT_FLOAT, _T("Weather: wind speed for {instance} (km/h)") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Weather.Locations"), H_LocationsList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("Weather.Forecast(*)"), H_ForecastTable, nullptr, _T("TIME"), _T("Weather: hourly weather/solar forecast curve") },
   { _T("Weather.ForecastEnsemble(*)"), H_EnsembleTable, nullptr, _T("TIME"), _T("Weather: ensemble forecast spread (solar/temperature)") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("WEATHER"), NETXMS_VERSION_STRING,
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
DECLARE_SUBAGENT_ENTRY_POINT(WEATHER)
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
