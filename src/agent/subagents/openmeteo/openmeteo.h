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
** File: openmeteo.h
**
**/

#ifndef _openmeteo_h_
#define _openmeteo_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

#define DEBUG_TAG _T("openmeteo")

#define OPENMETEO_FORECAST_URL   "https://api.open-meteo.com/v1/forecast"
#define OPENMETEO_ENSEMBLE_URL   "https://ensemble-api.open-meteo.com/v1/ensemble"
#define MAX_LOC_NAME             64
#define MAX_LOC_KEY              32

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
   RELATIVE_HUMIDITY
};

/**
 * Current weather snapshot for one location. Missing fields are NaN.
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
 * Open-Meteo API client. The API is keyless, so requests carry a full URL.
 */
class OpenMeteoClient
{
private:
   uint32_t m_timeout;     // request timeout, ms

public:
   OpenMeteoClient(uint32_t timeout) { m_timeout = timeout; }

   // Execute a GET for the given URL; returns true on HTTP 2xx and fills
   // response with the body. httpCode receives the HTTP status.
   bool get(const char *url, ByteStream *response, long *httpCode) const;
};

/**
 * Response parsers (adapter.cpp) - the only Open-Meteo-aware code. Each returns
 * a heap object on success or nullptr on failure (bad JSON, error document, no
 * usable data).
 */
bool ParseForecastResponse(const char *json, size_t len, WeatherSnapshot **current, ForecastCurve **forecast);
EnsembleCurve *ParseEnsembleResponse(const char *json, size_t len);

/**
 * A configured or lazily-resolved location with cached state.
 */
class MeteoLocation
{
private:
   TCHAR m_name[MAX_LOC_NAME];   // display name (equals "lat,lon" for raw instances)
   double m_latitude;
   double m_longitude;
   char m_key[MAX_LOC_KEY];      // canonical "lat,lon" for matching and cache dedup

   mutable Mutex m_lock;
   WeatherSnapshot *m_current;   // nullptr until first successful poll
   ForecastCurve *m_forecast;
   EnsembleCurve *m_ensemble;

public:
   MeteoLocation(const TCHAR *name, double latitude, double longitude);
   ~MeteoLocation();

   const TCHAR *getName() const { return m_name; }
   const char *getKey() const { return m_key; }
   double getLatitude() const { return m_latitude; }
   double getLongitude() const { return m_longitude; }

   void poll(const OpenMeteoClient& client, int forecastDays, bool ensemble, const char *ensembleModel);

   // Accessors: copy needed data out under lock. Return false when no data yet.
   bool getCurrent(WeatherField field, double *value) const;
   bool getDataAge(int64_t *seconds) const;
   bool fillForecastTable(Table *table) const;
   bool fillEnsembleTable(Table *table) const;
};

/**
 * Format a canonical location key from coordinates (adapter.cpp). ~11 m
 * precision, so a named location and an equal raw pair share one cache entry.
 */
void FormatLocationKey(double latitude, double longitude, char *buffer);

/**
 * Parse a "lat,lon" pair (adapter.cpp). Returns true and fills latitude/longitude
 * only when the whole string is two comma-separated numbers within valid ranges.
 */
bool ParseLatLon(const TCHAR *str, double *latitude, double *longitude);

#endif   /* _openmeteo_h_ */
