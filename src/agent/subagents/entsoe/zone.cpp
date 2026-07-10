/*
** NetXMS ENTSO-E Transparency Platform subagent
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
** File: zone.cpp
** Configured bidding zone with cached state and poll orchestration.
**
**/

#include "entsoe.h"
#include <math.h>

#define FORECAST_HORIZON_SEC   (48 * 3600)

/**
 * Convert an ASCII string to TCHAR (used for codes/names known to be ASCII).
 */
static inline void AsciiToTchar(const char *src, TCHAR *dst, size_t size)
{
#ifdef UNICODE
   size_t i = 0;
   for(; (src[i] != 0) && (i < size - 1); i++)
      dst[i] = static_cast<TCHAR>(static_cast<unsigned char>(src[i]));
   dst[i] = 0;
#else
   strlcpy(dst, src, size);
#endif
}

/**
 * Format a time as ENTSO-E period boundary (yyyyMMddHHmm, UTC).
 */
static void FormatPeriod(time_t t, char *buffer)
{
   struct tm tmbuf;
#ifdef _WIN32
   gmtime_s(&tmbuf, &t);
#else
   gmtime_r(&t, &tmbuf);
#endif
   snprintf(buffer, 16, "%04d%02d%02d%02d%02d",
      tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min);
}

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
EntsoeZone::EntsoeZone(const TCHAR *name, const char *eic) : m_borders(8, 8)
{
   _tcslcpy(m_name, name, MAX_ZONE_NAME);
   strlcpy(m_eic, eic, MAX_EIC_LEN);
   m_generation = nullptr;
   m_forecast = nullptr;
   m_flows = nullptr;
}

/**
 * Destructor
 */
EntsoeZone::~EntsoeZone()
{
   delete m_generation;
   delete m_forecast;
   delete m_flows;
}

/**
 * Add a cross-border neighbour.
 */
void EntsoeZone::addBorder(const char *name, const char *eic)
{
   Border b;
   strlcpy(b.name, name, MAX_ZONE_NAME);
   strlcpy(b.eic, eic, MAX_EIC_LEN);
   m_borders.add(&b);
}

/**
 * Poll all ENTSO-E data for this zone and update cached state. Each document is
 * updated independently: a failed request leaves the previous value in place.
 */
void EntsoeZone::poll(const EntsoeClient& client, const EmissionFactorSet& factors, uint32_t lookback)
{
   time_t now = time(nullptr);
   char periodStart[16], periodEnd[16];
   FormatPeriod(now - lookback, periodStart);
   FormatPeriod(now, periodEnd);

   char query[512];
   long httpCode;

   // Actual generation per production type (A75 / A16 Realised)
   {
      snprintf(query, sizeof(query),
         "documentType=A75&processType=A16&in_Domain=%s&periodStart=%s&periodEnd=%s",
         m_eic, periodStart, periodEnd);
      ByteStream response(32768);
      response.setAllocationStep(32768);
      if (client.get(query, &response, &httpCode))
      {
         GenerationSnapshot *g = ParseGenerationDocument(
            reinterpret_cast<const char*>(response.buffer()), response.size() - 1, factors);
         if (g != nullptr)
         {
            m_lock.lock();
            delete m_generation;
            m_generation = g;
            m_lock.unlock();
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Zone %s: generation updated, CI=%.1f gCO2/kWh, renewable=%.1f%%"),
               m_name, g->carbonIntensity, g->renewableShare);
         }
      }
   }

   // Wind/solar forecast (A69) + load forecast (A65) merged into one curve
   {
      char fStart[16], fEnd[16];
      FormatPeriod(now, fStart);
      FormatPeriod(now + FORECAST_HORIZON_SEC, fEnd);
      ForecastCurve *curve = new ForecastCurve();
      bool any = false;

      snprintf(query, sizeof(query),
         "documentType=A69&processType=A01&in_Domain=%s&periodStart=%s&periodEnd=%s",
         m_eic, fStart, fEnd);
      ByteStream wsResponse(16384);
      wsResponse.setAllocationStep(16384);
      if (client.get(query, &wsResponse, &httpCode) &&
          ParseForecastDocument(reinterpret_cast<const char*>(wsResponse.buffer()), wsResponse.size() - 1, curve, FORECAST_WINDSOLAR))
         any = true;

      snprintf(query, sizeof(query),
         "documentType=A65&processType=A01&outBiddingZone_Domain=%s&periodStart=%s&periodEnd=%s",
         m_eic, fStart, fEnd);
      ByteStream loadResponse(16384);
      loadResponse.setAllocationStep(16384);
      if (client.get(query, &loadResponse, &httpCode) &&
          ParseForecastDocument(reinterpret_cast<const char*>(loadResponse.buffer()), loadResponse.size() - 1, curve, FORECAST_LOAD))
         any = true;

      if (any)
      {
         curve->points.sort(ForecastPointComparator);
         m_lock.lock();
         delete m_forecast;
         m_forecast = curve;
         m_lock.unlock();
      }
      else
      {
         delete curve;
      }
   }

   // Cross-border physical flows (A11), both directions per border
   if (m_borders.size() > 0)
   {
      FlowSnapshot *flows = new FlowSnapshot();
      double netTotal = 0;
      bool anyFlow = false;
      for(int i = 0; i < m_borders.size(); i++)
      {
         Border *b = m_borders.get(i);
         bool ok1 = false, ok2 = false;
         double importMW = 0, exportMW = 0;

         // Import into this zone: in_Domain=this, out_Domain=neighbour
         snprintf(query, sizeof(query),
            "documentType=A11&in_Domain=%s&out_Domain=%s&periodStart=%s&periodEnd=%s",
            m_eic, b->eic, periodStart, periodEnd);
         ByteStream r1(8192);
         r1.setAllocationStep(8192);
         if (client.get(query, &r1, &httpCode))
            importMW = ParseLatestFlow(reinterpret_cast<const char*>(r1.buffer()), r1.size() - 1, &ok1);

         // Export from this zone: in_Domain=neighbour, out_Domain=this
         snprintf(query, sizeof(query),
            "documentType=A11&in_Domain=%s&out_Domain=%s&periodStart=%s&periodEnd=%s",
            b->eic, m_eic, periodStart, periodEnd);
         ByteStream r2(8192);
         r2.setAllocationStep(8192);
         if (client.get(query, &r2, &httpCode))
            exportMW = ParseLatestFlow(reinterpret_cast<const char*>(r2.buffer()), r2.size() - 1, &ok2);

         if (ok1 || ok2)
         {
            FlowValue fv;
            strlcpy(fv.neighbour, b->name, MAX_ZONE_NAME);
            fv.importMW = ok1 ? importMW : 0;
            fv.exportMW = ok2 ? exportMW : 0;
            fv.netMW = fv.importMW - fv.exportMW;
            flows->flows.add(&fv);
            netTotal += fv.netMW;
            anyFlow = true;
         }
      }
      if (anyFlow)
      {
         flows->netImportMW = netTotal;
         flows->intervalTime = now;
         m_lock.lock();
         delete m_flows;
         m_flows = flows;
         m_lock.unlock();
      }
      else
      {
         delete flows;
      }
   }
}

/**
 * Accessors
 */
bool EntsoeZone::getCarbonIntensity(double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;
   *value = m_generation->carbonIntensity;
   return true;
}

bool EntsoeZone::getShare(bool lowCarbon, double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;
   *value = lowCarbon ? m_generation->lowCarbonShare : m_generation->renewableShare;
   return true;
}

bool EntsoeZone::getTotalGeneration(double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;
   *value = m_generation->totalMW;
   return true;
}

bool EntsoeZone::getGenerationByType(const char *psrType, double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;
   for(int i = 0; i < m_generation->byType.size(); i++)
   {
      if (!stricmp(m_generation->byType.get(i)->psrType, psrType))
      {
         *value = m_generation->byType.get(i)->mw;
         return true;
      }
   }
   *value = 0;
   return true;
}

bool EntsoeZone::getNetImport(double *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_flows == nullptr)
      return false;
   *value = m_flows->netImportMW;
   return true;
}

bool EntsoeZone::getDataAge(int64_t *seconds) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;
   *seconds = static_cast<int64_t>(time(nullptr) - m_generation->intervalTime);
   return true;
}

bool EntsoeZone::fillGenerationTable(Table *table) const
{
   table->addColumn(_T("CODE"), DCI_DT_STRING, _T("Code"), true);
   table->addColumn(_T("NAME"), DCI_DT_STRING, _T("Production type"));
   table->addColumn(_T("CATEGORY"), DCI_DT_STRING, _T("Category"));
   table->addColumn(_T("MW"), DCI_DT_FLOAT, _T("Generation (MW)"));
   table->addColumn(_T("SHARE"), DCI_DT_FLOAT, _T("Share (%)"));

   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return false;

   double total = m_generation->totalMW;
   for(int i = 0; i < m_generation->byType.size(); i++)
   {
      GenerationValue *g = m_generation->byType.get(i);
      TCHAR code[MAX_PSR_TYPE];
      AsciiToTchar(g->psrType, code, MAX_PSR_TYPE);
      table->addRow();
      table->set(0, code);
      table->set(1, PsrTypeFriendlyName(g->psrType));
      table->set(2, CategoryName(g->category));
      table->set(3, g->mw);
      table->set(4, ((total > 0) && (g->category != GenCategory::STORAGE)) ? g->mw / total * 100.0 : 0.0);
   }
   return true;
}

bool EntsoeZone::fillFlowsTable(Table *table) const
{
   table->addColumn(_T("NEIGHBOUR"), DCI_DT_STRING, _T("Neighbour"), true);
   table->addColumn(_T("IMPORT"), DCI_DT_FLOAT, _T("Import (MW)"));
   table->addColumn(_T("EXPORT"), DCI_DT_FLOAT, _T("Export (MW)"));
   table->addColumn(_T("NET"), DCI_DT_FLOAT, _T("Net import (MW)"));

   LockGuard lockGuard(m_lock);
   if (m_flows == nullptr)
      return false;

   for(int i = 0; i < m_flows->flows.size(); i++)
   {
      FlowValue *fv = m_flows->flows.get(i);
      TCHAR neighbour[MAX_ZONE_NAME];
      AsciiToTchar(fv->neighbour, neighbour, MAX_ZONE_NAME);
      table->addRow();
      table->set(0, neighbour);
      table->set(1, fv->importMW);
      table->set(2, fv->exportMW);
      table->set(3, fv->netMW);
   }
   return true;
}

bool EntsoeZone::fillForecastTable(Table *table) const
{
   table->addColumn(_T("TIME"), DCI_DT_INT64, _T("Target time (UTC)"), true);
   table->addColumn(_T("WIND"), DCI_DT_FLOAT, _T("Wind (MW)"));
   table->addColumn(_T("SOLAR"), DCI_DT_FLOAT, _T("Solar (MW)"));
   table->addColumn(_T("LOAD"), DCI_DT_FLOAT, _T("Load (MW)"));

   LockGuard lockGuard(m_lock);
   if (m_forecast == nullptr)
      return false;

   for(int i = 0; i < m_forecast->points.size(); i++)
   {
      ForecastPoint *p = m_forecast->points.get(i);
      table->addRow();
      table->set(0, static_cast<int64_t>(p->targetTime));
      if (!isnan(p->windMW))
         table->set(1, p->windMW);
      if (!isnan(p->solarMW))
         table->set(2, p->solarMW);
      if (!isnan(p->loadMW))
         table->set(3, p->loadMW);
   }
   return true;
}

void EntsoeZone::fillGenerationTypes(StringList *list) const
{
   LockGuard lockGuard(m_lock);
   if (m_generation == nullptr)
      return;
   for(int i = 0; i < m_generation->byType.size(); i++)
   {
      TCHAR code[MAX_PSR_TYPE];
      AsciiToTchar(m_generation->byType.get(i)->psrType, code, MAX_PSR_TYPE);
      list->add(code);
   }
}
