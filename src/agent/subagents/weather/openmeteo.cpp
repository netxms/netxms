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
** File: openmeteo.cpp
** Open-Meteo provider adapter.
**
**/

#include "weather.h"
#include <math.h>

// Free (non-commercial) hosts. A commercial subscription serves the same
// protocol from "customer-" prefixed hosts and requires an apikey parameter.
#define OPENMETEO_FORECAST_URL            "https://api.open-meteo.com/v1/forecast"
#define OPENMETEO_ENSEMBLE_URL            "https://ensemble-api.open-meteo.com/v1/ensemble"
#define OPENMETEO_FORECAST_URL_CUSTOMER   "https://customer-api.open-meteo.com/v1/forecast"
#define OPENMETEO_ENSEMBLE_URL_CUSTOMER   "https://customer-ensemble-api.open-meteo.com/v1/ensemble"

// Variable set requested for both current conditions and hourly forecast.
#define OPENMETEO_VARIABLES   "temperature_2m,relative_humidity_2m,cloud_cover,wind_speed_10m,precipitation,shortwave_radiation,direct_radiation"

/**
 * Constructor
 */
OpenMeteoProvider::OpenMeteoProvider(const char *apiKey, const char *ensembleModel)
{
   strlcpy(m_apiKey, apiKey, sizeof(m_apiKey));
   strlcpy(m_ensembleModel, ensembleModel, sizeof(m_ensembleModel));
}

/**
 * Build URL for the combined current conditions + hourly forecast request.
 */
void OpenMeteoProvider::buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const
{
   int pos = snprintf(url, size,
      "%s?latitude=%.4f&longitude=%.4f&current=%s&hourly=%s&forecast_days=%d&timeformat=unixtime&timezone=GMT",
      (m_apiKey[0] != 0) ? OPENMETEO_FORECAST_URL_CUSTOMER : OPENMETEO_FORECAST_URL,
      latitude, longitude, OPENMETEO_VARIABLES, OPENMETEO_VARIABLES, forecastDays);
   if ((m_apiKey[0] != 0) && (pos > 0) && (static_cast<size_t>(pos) < size))
      snprintf(&url[pos], size - pos, "&apikey=%s", m_apiKey);
}

/**
 * Build URL for the ensemble spread request.
 */
void OpenMeteoProvider::buildEnsembleUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const
{
   int pos = snprintf(url, size,
      "%s?latitude=%.4f&longitude=%.4f&hourly=shortwave_radiation,temperature_2m&models=%s&forecast_days=%d&timeformat=unixtime&timezone=GMT",
      (m_apiKey[0] != 0) ? OPENMETEO_ENSEMBLE_URL_CUSTOMER : OPENMETEO_ENSEMBLE_URL,
      latitude, longitude, m_ensembleModel, forecastDays);
   if ((m_apiKey[0] != 0) && (pos > 0) && (static_cast<size_t>(pos) < size))
      snprintf(&url[pos], size - pos, "&apikey=%s", m_apiKey);
}

/**
 * Read a numeric field from a JSON object, returning NaN when absent or not a
 * number (Open-Meteo omits unavailable variables rather than sending null).
 */
static inline double JsonNumberOrNaN(json_t *object, const char *key)
{
   json_t *v = json_object_get(object, key);
   return json_is_number(v) ? json_number_value(v) : NAN;
}

/**
 * Value at index in a JSON number array, or NaN if out of range / not a number.
 */
static inline double ArrayValueOrNaN(json_t *array, size_t index)
{
   json_t *v = json_array_get(array, index);
   return json_is_number(v) ? json_number_value(v) : NAN;
}

/**
 * Detect an Open-Meteo error document ({"error":true,"reason":"..."}). Returns
 * true (and logs) if the document reports an error.
 */
static bool IsErrorDocument(json_t *root)
{
   if (!json_object_get_boolean(root, "error", false))
      return false;
   const char *reason = json_object_get_string_utf8(root, "reason", "(no reason)");
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Open-Meteo error document: %hs"), reason);
   return true;
}

/**
 * Parse a /v1/forecast response into a current snapshot and an hourly forecast
 * curve (a single request carries both). Either output may be left null if the
 * corresponding block is absent; returns false only on unusable input. The
 * horizon is applied server side, so forecastDays is not used here.
 */
bool OpenMeteoProvider::parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const
{
   *current = nullptr;
   *forecast = nullptr;

   json_error_t error;
   json_t *root = json_loadb(data, len, 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse forecast response (%hs at line %d)"), error.text, error.line);
      return false;
   }
   if (IsErrorDocument(root))
   {
      json_decref(root);
      return false;
   }

   // Current conditions
   json_t *cur = json_object_get(root, "current");
   if (json_is_object(cur))
   {
      WeatherSnapshot *s = MemAllocStruct<WeatherSnapshot>();
      s->observationTime = static_cast<time_t>(json_object_get_int64(cur, "time", 0));
      s->temperature = JsonNumberOrNaN(cur, "temperature_2m");
      s->cloudCover = JsonNumberOrNaN(cur, "cloud_cover");
      s->shortwaveRadiation = JsonNumberOrNaN(cur, "shortwave_radiation");
      s->directRadiation = JsonNumberOrNaN(cur, "direct_radiation");
      s->windSpeed = JsonNumberOrNaN(cur, "wind_speed_10m");
      s->relativeHumidity = JsonNumberOrNaN(cur, "relative_humidity_2m");
      s->precipitation = JsonNumberOrNaN(cur, "precipitation");
      *current = s;
   }

   // Hourly forecast curve
   json_t *hourly = json_object_get(root, "hourly");
   if (json_is_object(hourly))
   {
      json_t *timeArray = json_object_get(hourly, "time");
      if (json_is_array(timeArray))
      {
         json_t *temp = json_object_get(hourly, "temperature_2m");
         json_t *cloud = json_object_get(hourly, "cloud_cover");
         json_t *shortwave = json_object_get(hourly, "shortwave_radiation");
         json_t *direct = json_object_get(hourly, "direct_radiation");
         json_t *wind = json_object_get(hourly, "wind_speed_10m");
         json_t *humidity = json_object_get(hourly, "relative_humidity_2m");
         json_t *precipitation = json_object_get(hourly, "precipitation");

         ForecastCurve *curve = new ForecastCurve();
         size_t count = json_array_size(timeArray);
         for(size_t i = 0; i < count; i++)
         {
            json_t *t = json_array_get(timeArray, i);
            if (!json_is_integer(t))
               continue;
            ForecastPoint p;
            p.targetTime = static_cast<time_t>(json_integer_value(t));
            p.temperature = ArrayValueOrNaN(temp, i);
            p.cloudCover = ArrayValueOrNaN(cloud, i);
            p.shortwaveRadiation = ArrayValueOrNaN(shortwave, i);
            p.directRadiation = ArrayValueOrNaN(direct, i);
            p.windSpeed = ArrayValueOrNaN(wind, i);
            p.relativeHumidity = ArrayValueOrNaN(humidity, i);
            p.precipitation = ArrayValueOrNaN(precipitation, i);
            curve->points.add(&p);
         }
         if (curve->points.size() > 0)
            *forecast = curve;
         else
            delete curve;
      }
   }

   json_decref(root);
   return (*current != nullptr) || (*forecast != nullptr);
}

/**
 * Accumulate min/mean/max across ensemble member arrays for one variable.
 * The ensemble endpoint returns the control run under the base variable name and
 * members as "<base>_memberNN"; both forms are folded into the spread.
 */
struct SpreadAccumulator
{
   json_t *members[128];
   int count;

   void collect(json_t *hourly, const char *base)
   {
      count = 0;
      size_t baseLen = strlen(base);
      void *iter = json_object_iter(hourly);
      while((iter != nullptr) && (count < static_cast<int>(sizeof(members) / sizeof(members[0]))))
      {
         const char *key = json_object_iter_key(iter);
         json_t *value = json_object_iter_value(iter);
         if (json_is_array(value) &&
             !strncmp(key, base, baseLen) &&
             ((key[baseLen] == 0) || ((key[baseLen] == '_') && !strncmp(&key[baseLen + 1], "member", 6))))
            members[count++] = value;
         iter = json_object_iter_next(hourly, iter);
      }
   }

   bool valueAt(size_t index, double *minValue, double *meanValue, double *maxValue) const
   {
      double lo = NAN, hi = NAN, sum = 0;
      int n = 0;
      for(int i = 0; i < count; i++)
      {
         json_t *v = json_array_get(members[i], index);
         if (!json_is_number(v))
            continue;
         double d = json_number_value(v);
         if (n == 0)
         {
            lo = hi = d;
         }
         else
         {
            if (d < lo) lo = d;
            if (d > hi) hi = d;
         }
         sum += d;
         n++;
      }
      if (n == 0)
         return false;
      *minValue = lo;
      *maxValue = hi;
      *meanValue = sum / n;
      return true;
   }
};

/**
 * Parse an /v1/ensemble response into a per-hour spread (min/mean/max) of solar
 * irradiance and temperature across all ensemble members.
 */
EnsembleCurve *OpenMeteoProvider::parseEnsembleResponse(const char *data, size_t len) const
{
   json_error_t error;
   json_t *root = json_loadb(data, len, 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse ensemble response (%hs at line %d)"), error.text, error.line);
      return nullptr;
   }
   if (IsErrorDocument(root))
   {
      json_decref(root);
      return nullptr;
   }

   json_t *hourly = json_object_get(root, "hourly");
   json_t *timeArray = json_is_object(hourly) ? json_object_get(hourly, "time") : nullptr;
   if (!json_is_array(timeArray))
   {
      json_decref(root);
      return nullptr;
   }

   SpreadAccumulator solar, temp;
   solar.collect(hourly, "shortwave_radiation");
   temp.collect(hourly, "temperature_2m");

   EnsembleCurve *curve = new EnsembleCurve();
   size_t count = json_array_size(timeArray);
   for(size_t i = 0; i < count; i++)
   {
      json_t *t = json_array_get(timeArray, i);
      if (!json_is_integer(t))
         continue;
      EnsemblePoint p;
      p.targetTime = static_cast<time_t>(json_integer_value(t));
      if (!solar.valueAt(i, &p.solarMin, &p.solarMean, &p.solarMax))
      {
         p.solarMin = p.solarMean = p.solarMax = NAN;
      }
      if (!temp.valueAt(i, &p.tempMin, &p.tempMean, &p.tempMax))
      {
         p.tempMin = p.tempMean = p.tempMax = NAN;
      }
      curve->points.add(&p);
   }

   json_decref(root);
   if (curve->points.size() == 0)
   {
      delete curve;
      return nullptr;
   }
   return curve;
}
