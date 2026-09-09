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
** File: brightsky.cpp
** Bright Sky provider adapter: JSON front end to DWD open data (MOSMIX
** forecasts and station observations). Data licensed CC-BY 4.0 by DWD.
**
**/

#include "weather.h"
#include <math.h>

#define BRIGHTSKY_WEATHER_URL   "https://api.brightsky.dev/weather"

/**
 * Build URL for the /weather request. The window starts at 00:00 UTC of the
 * current day so the URL (and thus the conditional-request validators) stays
 * stable for a whole day; past hours of today come back as station observations
 * and future hours as MOSMIX forecast, mirroring Open-Meteo's "from midnight"
 * layout. Units are requested explicitly so a change of the service default
 * cannot silently alter the numbers.
 */
void BrightSkyProvider::buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const
{
   time_t now = time(nullptr);
   time_t startOfDay = now - (now % 86400);
   time_t lastDate = startOfDay + static_cast<time_t>(forecastDays) * 86400;

   struct tm start, end;
#if HAVE_GMTIME_R
   gmtime_r(&startOfDay, &start);
   gmtime_r(&lastDate, &end);
#else
   memcpy(&start, gmtime(&startOfDay), sizeof(struct tm));
   memcpy(&end, gmtime(&lastDate), sizeof(struct tm));
#endif

   snprintf(url, size, "%s?lat=%.4f&lon=%.4f&date=%04d-%02d-%02dT00:00:00Z&last_date=%04d-%02d-%02dT00:00:00Z&tz=Etc/UTC&units=dwd",
      BRIGHTSKY_WEATHER_URL, latitude, longitude,
      start.tm_year + 1900, start.tm_mon + 1, start.tm_mday,
      end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
}

/**
 * Read a numeric field from a JSON object, returning NaN when absent, null, or not a number.
 */
static inline double JsonNumberOrNaN(json_t *object, const char *key)
{
   json_t *v = json_object_get(object, key);
   return json_is_number(v) ? json_number_value(v) : NAN;
}

/**
 * Fill a forecast point from one weather record. Bright Sky's "dwd" unit system
 * already matches the model (°C, %, km/h, mm) except solar, which is the energy
 * received over the hour in kWh/m² and is converted to mean irradiance in W/m².
 * DWD publishes global irradiance only, so direct radiation is never available.
 */
static void FillPoint(ForecastPoint *p, json_t *record)
{
   p->temperature = JsonNumberOrNaN(record, "temperature");
   p->cloudCover = JsonNumberOrNaN(record, "cloud_cover");
   p->windSpeed = JsonNumberOrNaN(record, "wind_speed");
   p->relativeHumidity = JsonNumberOrNaN(record, "relative_humidity");
   p->precipitation = JsonNumberOrNaN(record, "precipitation");
   p->directRadiation = NAN;
   double solar = JsonNumberOrNaN(record, "solar");
   p->shortwaveRadiation = isnan(solar) ? NAN : solar * 1000.0;
}

/**
 * Parse a /weather response. Records form a single hourly series; the last
 * record at or before now (an observation when a station is in range) becomes
 * the current snapshot and the whole series becomes the forecast curve. An error
 * document ({"title": ..., "description": ...}, sent for example when no source
 * lies within max_dist) is reported and rejected.
 */
bool BrightSkyProvider::parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const
{
   *current = nullptr;
   *forecast = nullptr;

   json_error_t error;
   json_t *root = json_loadb(data, len, 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse Bright Sky response (%hs at line %d)"), error.text, error.line);
      return false;
   }

   json_t *records = json_object_get(root, "weather");
   if (!json_is_array(records))
   {
      const char *description = json_object_get_string_utf8(root, "description", nullptr);
      if (description != nullptr)
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Bright Sky error document: %hs"), description);
      else
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Bright Sky response has no weather records"));
      json_decref(root);
      return false;
   }

   time_t now = time(nullptr);

   ForecastCurve *curve = new ForecastCurve();
   ForecastPoint currentPoint;
   bool currentValid = false;
   size_t count = json_array_size(records);
   for(size_t i = 0; i < count; i++)
   {
      json_t *record = json_array_get(records, i);
      if (!json_is_object(record))
         continue;
      time_t targetTime = ParseIsoTimestamp(json_object_get_string_utf8(record, "timestamp", nullptr));
      if (targetTime == 0)
         continue;

      ForecastPoint p;
      p.targetTime = targetTime;
      FillPoint(&p, record);

      // The last record at or before now is the closest thing to an observation;
      // if the series starts in the future, its first record is used instead.
      if ((targetTime <= now) || !currentValid)
      {
         currentPoint = p;
         currentValid = true;
      }
      curve->points.add(&p);
   }

   json_decref(root);

   if (currentValid)
   {
      WeatherSnapshot *s = MemAllocStruct<WeatherSnapshot>();
      s->observationTime = currentPoint.targetTime;
      s->temperature = currentPoint.temperature;
      s->cloudCover = currentPoint.cloudCover;
      s->shortwaveRadiation = currentPoint.shortwaveRadiation;
      s->directRadiation = currentPoint.directRadiation;
      s->windSpeed = currentPoint.windSpeed;
      s->relativeHumidity = currentPoint.relativeHumidity;
      s->precipitation = currentPoint.precipitation;
      *current = s;
   }

   if (curve->points.size() > 0)
      *forecast = curve;
   else
      delete curve;

   return (*current != nullptr) || (*forecast != nullptr);
}
