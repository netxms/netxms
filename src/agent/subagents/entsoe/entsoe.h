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
** File: entsoe.h
**
**/

#ifndef _entsoe_h_
#define _entsoe_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

#define DEBUG_TAG _T("entsoe")

#define ENTSOE_API_URL          "https://web-api.tp.entsoe.eu/api"
#define MAX_ZONE_NAME           64
#define MAX_EIC_LEN             24
#define MAX_PSR_TYPE            8

/**
 * Production type category. Drives share metrics and CI treatment.
 */
enum class GenCategory
{
   RENEWABLE,
   NUCLEAR,
   FOSSIL,
   STORAGE,     // excluded from carbon-intensity and share denominators
   OTHER
};

const TCHAR *CategoryName(GenCategory c);
const TCHAR *PsrTypeFriendlyName(const char *code);

/**
 * Emission factor for one production type.
 */
struct EmissionFactor
{
   char code[MAX_PSR_TYPE];
   double gramsPerKWh;
   GenCategory category;
};

/**
 * Emission factor set: compiled-in defaults, optionally overridden from file.
 */
class EmissionFactorSet
{
private:
   StructArray<EmissionFactor> m_factors;
   double m_defaultFactor;

public:
   EmissionFactorSet();

   void loadDefaults();
   bool loadFromFile(const TCHAR *path);
   void setDefaultFactor(double f) { m_defaultFactor = f; }
   double defaultFactor() const { return m_defaultFactor; }

   // Returns factor entry for code, or nullptr if not mapped.
   const EmissionFactor *find(const char *code) const;
};

/**
 * Generation value for one production type in the current interval.
 */
struct GenerationValue
{
   char psrType[MAX_PSR_TYPE];
   double mw;
   GenCategory category;
   double factor;
};

/**
 * Actual generation snapshot for one zone (one served interval).
 */
struct GenerationSnapshot
{
   time_t intervalTime;                    // UTC start of served interval
   StructArray<GenerationValue> byType;
   double totalMW;                         // sum excluding storage
   double carbonIntensity;                 // gCO2/kWh
   double renewableShare;                  // percent of totalMW
   double lowCarbonShare;                  // percent of totalMW (renewable + nuclear)

   GenerationSnapshot() : byType(24, 24) { intervalTime = 0; totalMW = 0; carbonIntensity = 0; renewableShare = 0; lowCarbonShare = 0; }
};

/**
 * One point of the forward forecast curve.
 */
struct ForecastPoint
{
   time_t targetTime;
   double windMW;      // NaN if not available
   double solarMW;
   double loadMW;
};

/**
 * Forecast curve for one zone (wind/solar + load over the horizon).
 */
struct ForecastCurve
{
   StructArray<ForecastPoint> points;      // sorted by targetTime

   ForecastCurve() : points(48, 48) { }
};

/**
 * Cross-border flow with one neighbour.
 */
struct FlowValue
{
   char neighbour[MAX_ZONE_NAME];
   double importMW;
   double exportMW;
   double netMW;
};

/**
 * Cross-border flow snapshot for one zone.
 */
struct FlowSnapshot
{
   time_t intervalTime;
   double netImportMW;
   StructArray<FlowValue> flows;

   FlowSnapshot() : flows(8, 8) { intervalTime = 0; netImportMW = 0; }
};

/**
 * ENTSO-E API client: builds and executes token-authenticated requests.
 */
class EntsoeClient
{
private:
   char m_token[128];
   uint32_t m_timeout;     // request timeout, ms

public:
   EntsoeClient(const char *token, uint32_t timeout) { strlcpy(m_token, token, sizeof(m_token)); m_timeout = timeout; }

   // Execute a GET with the given query part (without token/URL); returns true on
   // HTTP 2xx and fills response with the body. httpCode receives the HTTP status.
   bool get(const char *query, ByteStream *response, long *httpCode) const;
};

/**
 * Document parsers (adapter.cpp). Each returns a heap object on success or
 * nullptr on failure (bad XML, acknowledgement document, no usable data).
 */
GenerationSnapshot *ParseGenerationDocument(const char *xml, size_t len, const EmissionFactorSet& factors);
bool ParseForecastDocument(const char *xml, size_t len, ForecastCurve *curve, int field);  // field: 0=wind/solar, 1=load
double ParseLatestFlow(const char *xml, size_t len, bool *ok);

// Forecast field selectors for ParseForecastDocument
#define FORECAST_WINDSOLAR  0
#define FORECAST_LOAD       1

/**
 * Compute carbon intensity and shares for a generation snapshot from a factor set.
 */
void ComputeCarbonMetrics(GenerationSnapshot *snapshot, const EmissionFactorSet& factors);

/**
 * A configured bidding zone with cached state.
 */
class EntsoeZone
{
private:
   struct Border
   {
      char name[MAX_ZONE_NAME];
      char eic[MAX_EIC_LEN];
   };

   TCHAR m_name[MAX_ZONE_NAME];
   char m_eic[MAX_EIC_LEN];
   StructArray<Border> m_borders;

   mutable Mutex m_lock;
   GenerationSnapshot *m_generation;   // nullptr until first successful poll
   ForecastCurve *m_forecast;
   FlowSnapshot *m_flows;

public:
   EntsoeZone(const TCHAR *name, const char *eic);
   ~EntsoeZone();

   const TCHAR *getName() const { return m_name; }
   const char *getEic() const { return m_eic; }
   void addBorder(const char *name, const char *eic);

   void poll(const EntsoeClient& client, const EmissionFactorSet& factors, uint32_t lookback);

   // Accessors: copy needed data out under lock. Return false when no data yet.
   bool getCarbonIntensity(double *value) const;
   bool getShare(bool lowCarbon, double *value) const;
   bool getTotalGeneration(double *value) const;
   bool getGenerationByType(const char *psrType, double *value) const;
   bool getNetImport(double *value) const;
   bool getDataAge(int64_t *seconds) const;
   bool fillGenerationTable(Table *table) const;
   bool fillFlowsTable(Table *table) const;
   bool fillForecastTable(Table *table) const;
   void fillGenerationTypes(StringList *list) const;
};

/**
 * Zone catalog (catalog.cpp): resolve zone name to EIC + neighbours.
 */
struct CatalogZone
{
   char name[MAX_ZONE_NAME];
   char eic[MAX_EIC_LEN];
   StringList neighbours;   // neighbour zone names
};

void InitZoneCatalog();
bool LoadZoneCatalogFile(const TCHAR *path);
const CatalogZone *LookupCatalogZone(const char *name);
const char *LookupZoneEic(const char *name);

// Fetch/parse helpers shared with tests
time_t ParseIso8601Utc(const char *str);
int ParseIso8601Duration(const char *str);   // seconds; 0 on failure

#endif   /* _entsoe_h_ */
