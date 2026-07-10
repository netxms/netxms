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
** File: adapter.cpp
** Open-Meteo API client and JSON parsing (the only Open-Meteo-aware code).
**
**/

#include "openmeteo.h"
#include <netxms-version.h>
#include <nxlibcurl.h>
#include <math.h>

/**
 * Format a canonical location key from coordinates.
 */
void FormatLocationKey(double latitude, double longitude, char *buffer)
{
   snprintf(buffer, MAX_LOC_KEY, "%.4f,%.4f", latitude, longitude);
}

/**
 * Parse a "lat,lon" pair. Returns true only when the whole string is two
 * comma-separated numbers within valid ranges, so location names (which never
 * take this shape) fall through to catalog lookup.
 */
bool ParseLatLon(const TCHAR *str, double *latitude, double *longitude)
{
   if ((str == nullptr) || (*str == 0))
      return false;

   TCHAR *end;
   double lat = _tcstod(str, &end);
   if (end == str)
      return false;
   while((*end == ' ') || (*end == '\t'))
      end++;
   if (*end != _T(','))
      return false;
   const TCHAR *p = end + 1;
   double lon = _tcstod(p, &end);
   if (end == p)
      return false;
   while((*end == ' ') || (*end == '\t'))
      end++;
   if (*end != 0)
      return false;

   if ((lat < -90.0) || (lat > 90.0) || (lon < -180.0) || (lon > 180.0))
      return false;

   *latitude = lat;
   *longitude = lon;
   return true;
}

/**
 * cURL write callback: accumulate response into a ByteStream.
 */
static size_t CurlWriteFunction(char *ptr, size_t size, size_t nmemb, ByteStream *data)
{
   size_t bytes = size * nmemb;
   data->write(ptr, bytes);
   return bytes;
}

/**
 * Execute an API GET request.
 */
bool OpenMeteoClient::get(const char *url, ByteStream *response, long *httpCode) const
{
   *httpCode = 0;

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("curl_easy_init() failed"));
      return false;
   }

   char errorText[CURL_ERROR_SIZE];
   errorText[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);
#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1));
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(m_timeout));
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));
   curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(4));
   curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Agent/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(1));
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, static_cast<long>(2));
   EnableLibCURLUnexpectedEOFWorkaround(curl);

   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

   bool success = false;
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, httpCode);
         if ((*httpCode >= 200) && (*httpCode <= 299))
         {
            response->write('\0');
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("HTTP response code %03ld for [%hs]"), *httpCode, url);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("curl_easy_perform() failed (%hs)"), errorText);
      }
   }
   curl_easy_cleanup(curl);
   return success;
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
 * corresponding block is absent; returns false only on unusable input.
 */
bool ParseForecastResponse(const char *json, size_t len, WeatherSnapshot **current, ForecastCurve **forecast)
{
   *current = nullptr;
   *forecast = nullptr;

   json_error_t error;
   json_t *root = json_loadb(json, len, 0, &error);
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
EnsembleCurve *ParseEnsembleResponse(const char *json, size_t len)
{
   json_error_t error;
   json_t *root = json_loadb(json, len, 0, &error);
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
