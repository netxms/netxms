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
** File: metno.cpp
** MET Norway Locationforecast 2.0 provider adapter. Data licensed CC-BY 4.0.
**
**/

#include "weather.h"
#include <math.h>

#define METNO_FORECAST_URL   "https://api.met.no/weatherapi/locationforecast/2.0/compact"

/**
 * Build URL for the Locationforecast request. The service always returns its
 * full horizon (no per-request day count), and their terms of service cap
 * coordinates at four decimals.
 */
void MetNoProvider::buildForecastUrl(double latitude, double longitude, int forecastDays, char *url, size_t size) const
{
   snprintf(url, size, "%s?lat=%.4f&lon=%.4f", METNO_FORECAST_URL, latitude, longitude);
}

/**
 * Read a numeric field from a JSON object, returning NaN when absent or not a number.
 */
static inline double JsonNumberOrNaN(json_t *object, const char *key)
{
   json_t *v = json_object_get(object, key);
   return json_is_number(v) ? json_number_value(v) : NAN;
}

/**
 * Extract the "next_1_hours" precipitation amount (mm) for one timeseries entry,
 * or NaN when the entry has no hourly block (beyond the hourly part of the horizon).
 */
static double GetPrecipitation(json_t *data)
{
   json_t *block = json_object_get(data, "next_1_hours");
   if (!json_is_object(block))
      return NAN;
   json_t *details = json_object_get(block, "details");
   return json_is_object(details) ? JsonNumberOrNaN(details, "precipitation_amount") : NAN;
}

/**
 * Fill a forecast point from the "instant" details of one timeseries entry.
 * Wind speed is reported in m/s and converted to the model's km/h.
 */
static void FillPoint(ForecastPoint *p, json_t *data)
{
   p->temperature = NAN;
   p->cloudCover = NAN;
   p->windSpeed = NAN;
   p->relativeHumidity = NAN;
   p->shortwaveRadiation = NAN;   // not provided by Locationforecast
   p->directRadiation = NAN;
   p->precipitation = GetPrecipitation(data);

   json_t *instant = json_object_get(data, "instant");
   json_t *details = json_is_object(instant) ? json_object_get(instant, "details") : nullptr;
   if (!json_is_object(details))
      return;

   p->temperature = JsonNumberOrNaN(details, "air_temperature");
   p->cloudCover = JsonNumberOrNaN(details, "cloud_area_fraction");
   p->relativeHumidity = JsonNumberOrNaN(details, "relative_humidity");
   double wind = JsonNumberOrNaN(details, "wind_speed");
   if (!isnan(wind))
      p->windSpeed = wind * 3.6;
}

/**
 * Parse a Locationforecast 2.0 response. The document is a single timeseries;
 * the entry covering the present moment becomes the current snapshot and the
 * whole series (trimmed to the configured horizon) becomes the forecast curve.
 */
bool MetNoProvider::parseForecastResponse(const char *data, size_t len, int forecastDays, WeatherSnapshot **current, ForecastCurve **forecast) const
{
   *current = nullptr;
   *forecast = nullptr;

   json_error_t error;
   json_t *root = json_loadb(data, len, 0, &error);
   if (root == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse Locationforecast response (%hs at line %d)"), error.text, error.line);
      return false;
   }

   json_t *properties = json_object_get(root, "properties");
   json_t *timeseries = json_is_object(properties) ? json_object_get(properties, "timeseries") : nullptr;
   if (!json_is_array(timeseries))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Locationforecast response has no timeseries"));
      json_decref(root);
      return false;
   }

   time_t now = time(nullptr);
   time_t horizon = now + static_cast<time_t>(forecastDays) * 86400;

   ForecastCurve *curve = new ForecastCurve();
   ForecastPoint currentPoint;
   bool currentValid = false;
   size_t count = json_array_size(timeseries);
   for(size_t i = 0; i < count; i++)
   {
      json_t *entry = json_array_get(timeseries, i);
      if (!json_is_object(entry))
         continue;
      time_t targetTime = ParseIsoTimestamp(json_object_get_string_utf8(entry, "time", nullptr));
      if (targetTime == 0)
         continue;
      json_t *entryData = json_object_get(entry, "data");
      if (!json_is_object(entryData))
         continue;

      ForecastPoint p;
      p.targetTime = targetTime;
      FillPoint(&p, entryData);

      // The last entry at or before now is the closest thing to an observation;
      // if the series starts in the future, its first entry is used instead.
      if ((targetTime <= now) || !currentValid)
      {
         currentPoint = p;
         currentValid = true;
      }

      if (targetTime <= horizon)
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
