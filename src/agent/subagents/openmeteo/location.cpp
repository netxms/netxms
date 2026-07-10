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
** File: location.cpp
** Configured location with cached state and poll orchestration.
**
**/

#include "openmeteo.h"
#include <math.h>

// Variable set requested for both current conditions and hourly forecast.
#define OPENMETEO_VARIABLES   "temperature_2m,relative_humidity_2m,cloud_cover,wind_speed_10m,shortwave_radiation,direct_radiation"

/**
 * Comparator for sorting forecast points by target time.
 */
static int ForecastPointComparator(const void *a, const void *b)
{
   time_t ta = static_cast<const ForecastPoint*>(a)->targetTime;
   time_t tb = static_cast<const ForecastPoint*>(b)->targetTime;
   return (ta < tb) ? -1 : ((ta > tb) ? 1 : 0);
}

/**
 * Constructor
 */
MeteoLocation::MeteoLocation(const TCHAR *name, double latitude, double longitude)
{
   _tcslcpy(m_name, name, MAX_LOC_NAME);
   m_latitude = latitude;
   m_longitude = longitude;
   FormatLocationKey(latitude, longitude, m_key);
   m_current = nullptr;
   m_forecast = nullptr;
   m_ensemble = nullptr;
}

/**
 * Destructor
 */
MeteoLocation::~MeteoLocation()
{
   MemFree(m_current);
   delete m_forecast;
   delete m_ensemble;
}

/**
 * Poll Open-Meteo data for this location and update cached state. Each request
 * is applied independently: a failed request leaves the previous value in place.
 */
void MeteoLocation::poll(const OpenMeteoClient& client, int forecastDays, bool ensemble, const char *ensembleModel)
{
   char url[1024];
   long httpCode;

   // Current conditions + hourly forecast (single request carries both)
   snprintf(url, sizeof(url),
      "%s?latitude=%.4f&longitude=%.4f&current=%s&hourly=%s&forecast_days=%d&timeformat=unixtime&timezone=GMT",
      OPENMETEO_FORECAST_URL, m_latitude, m_longitude, OPENMETEO_VARIABLES, OPENMETEO_VARIABLES, forecastDays);
   ByteStream response(32768);
   response.setAllocationStep(32768);
   if (client.get(url, &response, &httpCode))
   {
      WeatherSnapshot *current;
      ForecastCurve *forecast;
      if (ParseForecastResponse(reinterpret_cast<const char*>(response.buffer()), response.size() - 1, &current, &forecast))
      {
         if (forecast != nullptr)
            forecast->points.sort(ForecastPointComparator);
         m_lock.lock();
         if (current != nullptr)
         {
            MemFree(m_current);
            m_current = current;
         }
         if (forecast != nullptr)
         {
            delete m_forecast;
            m_forecast = forecast;
         }
         m_lock.unlock();
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Location %s (%hs): forecast updated (%d hourly points)"),
            m_name, m_key, (forecast != nullptr) ? forecast->points.size() : 0);
      }
   }

   // Ensemble spread (optional; separate endpoint)
   if (ensemble)
   {
      snprintf(url, sizeof(url),
         "%s?latitude=%.4f&longitude=%.4f&hourly=shortwave_radiation,temperature_2m&models=%s&forecast_days=%d&timeformat=unixtime&timezone=GMT",
         OPENMETEO_ENSEMBLE_URL, m_latitude, m_longitude, ensembleModel, forecastDays);
      ByteStream ensembleResponse(65536);
      ensembleResponse.setAllocationStep(65536);
      if (client.get(url, &ensembleResponse, &httpCode))
      {
         EnsembleCurve *curve = ParseEnsembleResponse(
            reinterpret_cast<const char*>(ensembleResponse.buffer()), ensembleResponse.size() - 1);
         if (curve != nullptr)
         {
            m_lock.lock();
            delete m_ensemble;
            m_ensemble = curve;
            m_lock.unlock();
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Location %s (%hs): ensemble updated (%d points)"),
               m_name, m_key, curve->points.size());
         }
      }
   }
}

/**
 * Get a current-conditions scalar field.
 */
bool MeteoLocation::getCurrent(WeatherField field, double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_current == nullptr)
      return false;

   double v;
   switch(field)
   {
      case WeatherField::TEMPERATURE:         v = m_current->temperature; break;
      case WeatherField::CLOUD_COVER:         v = m_current->cloudCover; break;
      case WeatherField::SHORTWAVE_RADIATION: v = m_current->shortwaveRadiation; break;
      case WeatherField::DIRECT_RADIATION:    v = m_current->directRadiation; break;
      case WeatherField::WIND_SPEED:          v = m_current->windSpeed; break;
      case WeatherField::RELATIVE_HUMIDITY:   v = m_current->relativeHumidity; break;
      default:                                return false;
   }
   if (isnan(v))
      return false;
   *value = v;
   return true;
}

/**
 * Get age of the current observation in seconds.
 */
bool MeteoLocation::getDataAge(int64_t *seconds) const
{
   LockGuard lockGuard(m_lock);
   if (m_current == nullptr)
      return false;
   *seconds = static_cast<int64_t>(time(nullptr) - m_current->observationTime);
   return true;
}

/**
 * Fill the hourly forecast table.
 */
bool MeteoLocation::fillForecastTable(Table *table) const
{
   table->addColumn(_T("TIME"), DCI_DT_INT64, _T("Target time (UTC)"), true);
   table->addColumn(_T("TEMPERATURE"), DCI_DT_FLOAT, _T("Temperature (C)"));
   table->addColumn(_T("CLOUDCOVER"), DCI_DT_FLOAT, _T("Cloud cover (%)"));
   table->addColumn(_T("SHORTWAVE"), DCI_DT_FLOAT, _T("Shortwave radiation (W/m2)"));
   table->addColumn(_T("DIRECT"), DCI_DT_FLOAT, _T("Direct radiation (W/m2)"));
   table->addColumn(_T("WINDSPEED"), DCI_DT_FLOAT, _T("Wind speed (km/h)"));
   table->addColumn(_T("HUMIDITY"), DCI_DT_FLOAT, _T("Relative humidity (%)"));

   LockGuard lockGuard(m_lock);
   if (m_forecast == nullptr)
      return false;

   for(int i = 0; i < m_forecast->points.size(); i++)
   {
      ForecastPoint *p = m_forecast->points.get(i);
      table->addRow();
      table->set(0, static_cast<int64_t>(p->targetTime));
      if (!isnan(p->temperature))
         table->set(1, p->temperature);
      if (!isnan(p->cloudCover))
         table->set(2, p->cloudCover);
      if (!isnan(p->shortwaveRadiation))
         table->set(3, p->shortwaveRadiation);
      if (!isnan(p->directRadiation))
         table->set(4, p->directRadiation);
      if (!isnan(p->windSpeed))
         table->set(5, p->windSpeed);
      if (!isnan(p->relativeHumidity))
         table->set(6, p->relativeHumidity);
   }
   return true;
}

/**
 * Fill the ensemble spread table.
 */
bool MeteoLocation::fillEnsembleTable(Table *table) const
{
   table->addColumn(_T("TIME"), DCI_DT_INT64, _T("Target time (UTC)"), true);
   table->addColumn(_T("SOLAR_MIN"), DCI_DT_FLOAT, _T("Shortwave radiation min (W/m2)"));
   table->addColumn(_T("SOLAR_MEAN"), DCI_DT_FLOAT, _T("Shortwave radiation mean (W/m2)"));
   table->addColumn(_T("SOLAR_MAX"), DCI_DT_FLOAT, _T("Shortwave radiation max (W/m2)"));
   table->addColumn(_T("TEMP_MIN"), DCI_DT_FLOAT, _T("Temperature min (C)"));
   table->addColumn(_T("TEMP_MEAN"), DCI_DT_FLOAT, _T("Temperature mean (C)"));
   table->addColumn(_T("TEMP_MAX"), DCI_DT_FLOAT, _T("Temperature max (C)"));

   LockGuard lockGuard(m_lock);
   if (m_ensemble == nullptr)
      return false;

   for(int i = 0; i < m_ensemble->points.size(); i++)
   {
      EnsemblePoint *p = m_ensemble->points.get(i);
      table->addRow();
      table->set(0, static_cast<int64_t>(p->targetTime));
      if (!isnan(p->solarMin))
         table->set(1, p->solarMin);
      if (!isnan(p->solarMean))
         table->set(2, p->solarMean);
      if (!isnan(p->solarMax))
         table->set(3, p->solarMax);
      if (!isnan(p->tempMin))
         table->set(4, p->tempMin);
      if (!isnan(p->tempMean))
         table->set(5, p->tempMean);
      if (!isnan(p->tempMax))
         table->set(6, p->tempMax);
   }
   return true;
}
