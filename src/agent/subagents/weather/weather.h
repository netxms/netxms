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
** File: weather.h
**
**/

#ifndef _weather_h_
#define _weather_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

#define DEBUG_TAG _T("weather")

#define MAX_LOC_NAME             64
#define MAX_LOC_KEY              64
#define MAX_PROVIDER_NAME        16
#define MAX_USER_AGENT           256
#define MAX_API_KEY              128

/**
 * Selector for a current-conditions scalar field.
 */
enum class WeatherField
{
   TEMPERATURE,
   CLOUD_COVER,
   SHORTWAVE_RADIATION,
   DIRECT_RADIATION,
   WIND_SPEED,
   RELATIVE_HUMIDITY,
   PRECIPITATION
};

/**
 * Current weather snapshot for one location. Missing fields are NaN. Units are
 * normalized across providers by the adapters.
 */
struct WeatherSnapshot
{
   time_t observationTime;
   double temperature;           // degrees C
   double cloudCover;            // percent
   double shortwaveRadiation;    // W/m2 (global horizontal irradiance)
   double directRadiation;       // W/m2
   double windSpeed;             // km/h
   double relativeHumidity;      // percent
   double precipitation;         // mm
};

/**
 * One point of the hourly forecast curve. Missing fields are NaN.
 */
struct ForecastPoint
{
   time_t targetTime;            // UTC
   double temperature;
   double cloudCover;
   double shortwaveRadiation;
   double directRadiation;
   double windSpeed;
   double relativeHumidity;
   double precipitation;
};

/**
 * Hourly forecast curve for one location.
 */
struct ForecastCurve
{
   StructArray<ForecastPoint> points;   // sorted by targetTime

   ForecastCurve() : points(48, 48) { }
};

/**
 * One point of the ensemble spread curve: min/mean/max across ensemble members
 * for the decision-relevant fields (solar irradiance and temperature).
 */
struct EnsemblePoint
{
   time_t targetTime;
   double solarMin, solarMean, solarMax;    // shortwave radiation, W/m2
   double tempMin, tempMean, tempMax;       // degrees C
};

/**
 * Ensemble spread curve for one location.
 */
struct EnsembleCurve
{
   StructArray<EnsemblePoint> points;

   EnsembleCurve() : points(48, 48) { }
};

/**
 * Validators from the last successful response for a single request. Kept per
 * location and request kind so a refresh can be conditional. Provider-neutral:
 * both supported APIs send these headers, and MET Norway's terms of service
 * require honoring them.
 */
struct HttpCacheState
{
   char lastModified[64];     // raw Last-Modified header value, empty if none
   time_t expires;            // parsed Expires header, 0 if none

   HttpCacheState()
   {
      lastModified[0] = 0;
      expires = 0;
   }

   bool isFresh(time_t now) const { return (expires != 0) && (now < expires); }
};

/**
 * Outcome of a conditional GET.
 */
enum class HttpRequestResult
{
   SUCCESS,          // 2xx, body written to response
   NOT_MODIFIED,     // 304, cached data still current
   FAILURE
};

/**
 * HTTP client for weather provider APIs. Requests carry a full URL; the
 * User-Agent is shared by all providers because MET Norway rejects generic ones.
 */
class HttpClient
{
private:
   uint32_t m_timeout;                    // request timeout, ms
   char m_userAgent[MAX_USER_AGENT];

public:
   HttpClient(uint32_t timeout, const char *userAgent);

   // Execute a GET for the given URL, sending If-Modified-Since when the cache
   // state carries a validator and updating it from the response headers.
   HttpRequestResult get(const char *url, HttpCacheState *cache, ByteStream *response) const;
};

/**
 * Provider adapter: builds request URLs and parses provider-specific responses
 * into the neutral data model. Adapters hold only immutable configuration and
 * are shared by all locations, so all methods must be thread-safe.
 */
class WeatherProvider
{
public:
   virtual ~WeatherProvider() { }

   virtual const char *getName() const = 0;

   virtual void buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const = 0;
   virtual bool parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const = 0;

   // Ensemble spread is an Open-Meteo capability; providers without it keep these defaults.
   virtual bool supportsEnsemble() const { return false; }
   virtual void buildEnsembleUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const { *url = 0; }
   virtual EnsembleCurve *parseEnsembleResponse(const char *data, size_t len) const { return nullptr; }
};

/**
 * Open-Meteo adapter (openmeteo.cpp). With an API key set, requests go to the
 * commercial ("customer-" prefixed) hosts and carry the key.
 */
class OpenMeteoProvider : public WeatherProvider
{
private:
   char m_apiKey[MAX_API_KEY];
   char m_ensembleModel[64];

public:
   OpenMeteoProvider(const char *apiKey, const char *ensembleModel);

   virtual const char *getName() const override { return "openmeteo"; }

   virtual void buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const override;
   virtual bool parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const override;

   virtual bool supportsEnsemble() const override { return true; }
   virtual void buildEnsembleUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const override;
   virtual EnsembleCurve *parseEnsembleResponse(const char *data, size_t len) const override;
};

/**
 * MET Norway Locationforecast 2.0 adapter (metno.cpp). Open data under CC-BY 4.0;
 * no key, but an identifying User-Agent is mandatory. Provides no solar
 * irradiance and no ensemble spread.
 */
class MetNoProvider : public WeatherProvider
{
public:
   virtual const char *getName() const override { return "metno"; }

   virtual void buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const override;
   virtual bool parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const override;
};

/**
 * A configured or lazily-resolved location with cached state.
 */
class WeatherLocation
{
private:
   TCHAR m_name[MAX_LOC_NAME];   // display name (equals "lat,lon" for raw instances)
   double m_latitude;
   double m_longitude;
   const WeatherProvider *m_provider;
   char m_key[MAX_LOC_KEY];      // canonical "provider:lat,lon" for matching and cache dedup

   mutable Mutex m_lock;
   WeatherSnapshot *m_current;   // nullptr until first successful poll
   ForecastCurve *m_forecast;
   EnsembleCurve *m_ensemble;
   HttpCacheState m_forecastCache;   // touched by the poller thread only
   HttpCacheState m_ensembleCache;

public:
   WeatherLocation(const TCHAR *name, double latitude, double longitude, const WeatherProvider *provider);
   ~WeatherLocation();

   const TCHAR *getName() const { return m_name; }
   const char *getKey() const { return m_key; }
   double getLatitude() const { return m_latitude; }
   double getLongitude() const { return m_longitude; }
   const WeatherProvider *getProvider() const { return m_provider; }

   void poll(const HttpClient& client, int forecastDays, bool ensemble);

   // Accessors: copy needed data out under lock. Return false when no data yet.
   bool getCurrent(WeatherField field, double *value) const;
   bool getDataAge(int64_t *seconds) const;
   bool fillForecastTable(Table *table) const;
   bool fillEnsembleTable(Table *table) const;
};

/**
 * Format a canonical location key from provider and coordinates (util.cpp).
 * Coordinates are kept at ~11 m precision, so a named location and an equal raw
 * pair on the same provider share one cache entry.
 */
void FormatLocationKey(const char *providerName, double latitude, double longitude, char *buffer);

/**
 * Parse a "lat,lon" pair (util.cpp). Returns true and fills latitude/longitude
 * only when the whole string is two comma-separated numbers within valid ranges.
 */
bool ParseLatLon(const TCHAR *str, double *latitude, double *longitude);

/**
 * Parse an ISO 8601 UTC timestamp ("2026-08-20T12:00:00Z") into time_t (util.cpp).
 * Returns 0 if the string does not have that shape.
 */
time_t ParseIsoTimestamp(const char *str);

#endif   /* _weather_h_ */
