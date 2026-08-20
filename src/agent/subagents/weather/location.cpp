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
** File: location.cpp
** Configured location with cached state and poll orchestration.
**
**/

#include "weather.h"
#include <math.h>

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
WeatherLocation::WeatherLocation(const TCHAR *name, double latitude, double longitude, const WeatherProvider *provider)
{
   _tcslcpy(m_name, name, MAX_LOC_NAME);
   m_latitude = latitude;
   m_longitude = longitude;
   m_provider = provider;
   FormatLocationKey(provider->getName(), latitude, longitude, m_key);
   m_current = nullptr;
   m_forecast = nullptr;
   m_ensemble = nullptr;
}

/**
 * Destructor
 */
WeatherLocation::~WeatherLocation()
{
   MemFree(m_current);
   delete m_forecast;
   delete m_ensemble;
}

/**
 * Poll provider data for this location and update cached state. Each request is
 * applied independently: a failed request leaves the previous value in place. A
 * response that is still within its Expires window is not re-requested at all,
 * and a refresh past that window is conditional on the stored Last-Modified.
 */
void WeatherLocation::poll(const HttpClient& client, int forecastDays, bool ensemble)
{
   char url[1024];
   time_t now = time(nullptr);

   // Current conditions + hourly forecast (single request carries both)
   if (m_forecastCache.isFresh(now))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Location %s (%hs): forecast still fresh, request skipped"), m_name, m_key);
   }
   else
   {
      m_provider->buildForecastUrl(m_latitude, m_longitude, forecastDays, url, sizeof(url));
      ByteStream response(32768);
      response.setAllocationStep(32768);
      HttpRequestResult result = client.get(url, &m_forecastCache, &response);
      if (result == HttpRequestResult::SUCCESS)
      {
         WeatherSnapshot *current;
         ForecastCurve *forecast;
         if (m_provider->parseForecastResponse(reinterpret_cast<const char*>(response.buffer()), response.size() - 1, forecastDays, &current, &forecast))
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
      else if (result == HttpRequestResult::NOT_MODIFIED)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Location %s (%hs): forecast not modified"), m_name, m_key);
      }
   }

   // Ensemble spread (optional; separate endpoint, not offered by every provider)
   if (!ensemble || !m_provider->supportsEnsemble())
      return;

   if (m_ensembleCache.isFresh(now))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Location %s (%hs): ensemble still fresh, request skipped"), m_name, m_key);
   }
   else
   {
      m_provider->buildEnsembleUrl(m_latitude, m_longitude, forecastDays, url, sizeof(url));
      ByteStream ensembleResponse(65536);
      ensembleResponse.setAllocationStep(65536);
      HttpRequestResult result = client.get(url, &m_ensembleCache, &ensembleResponse);
      if (result == HttpRequestResult::SUCCESS)
      {
         EnsembleCurve *curve = m_provider->parseEnsembleResponse(
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
      else if (result == HttpRequestResult::NOT_MODIFIED)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Location %s (%hs): ensemble not modified"), m_name, m_key);
      }
   }
}

/**
 * Get a current-conditions scalar field.
 */
bool WeatherLocation::getCurrent(WeatherField field, double *value) const
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
      case WeatherField::PRECIPITATION:       v = m_current->precipitation; break;
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
bool WeatherLocation::getDataAge(int64_t *seconds) const
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
bool WeatherLocation::fillForecastTable(Table *table) const
{
   table->addColumn(_T("TIME"), DCI_DT_INT64, _T("Target time (UTC)"), true);
   table->addColumn(_T("TEMPERATURE"), DCI_DT_FLOAT, _T("Temperature (C)"));
   table->addColumn(_T("CLOUDCOVER"), DCI_DT_FLOAT, _T("Cloud cover (%)"));
   table->addColumn(_T("SHORTWAVE"), DCI_DT_FLOAT, _T("Shortwave radiation (W/m2)"));
   table->addColumn(_T("DIRECT"), DCI_DT_FLOAT, _T("Direct radiation (W/m2)"));
   table->addColumn(_T("WINDSPEED"), DCI_DT_FLOAT, _T("Wind speed (km/h)"));
   table->addColumn(_T("HUMIDITY"), DCI_DT_FLOAT, _T("Relative humidity (%)"));
   table->addColumn(_T("PRECIPITATION"), DCI_DT_FLOAT, _T("Precipitation (mm)"));

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
      if (!isnan(p->precipitation))
         table->set(7, p->precipitation);
   }
   return true;
}

/**
 * Fill the ensemble spread table.
 */
bool WeatherLocation::fillEnsembleTable(Table *table) const
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
