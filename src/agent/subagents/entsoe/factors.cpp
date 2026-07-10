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
** File: factors.cpp
** Emission factors and carbon-intensity computation.
**
**/

#include "entsoe.h"

/**
 * Default emission factors (gCO2eq/kWh), lifecycle medians based on IPCC AR5.
 * These are the assumptions the derived carbon intensity depends on; they can
 * be overridden via the EmissionFactors= configuration file.
 */
static const EmissionFactor s_defaultFactors[] =
{
   { "B01",  230, GenCategory::RENEWABLE },   // Biomass (contested)
   { "B02", 1050, GenCategory::FOSSIL    },   // Fossil Brown coal/Lignite
   { "B03",  820, GenCategory::FOSSIL    },   // Fossil Coal-derived gas
   { "B04",  490, GenCategory::FOSSIL    },   // Fossil Gas
   { "B05",  820, GenCategory::FOSSIL    },   // Fossil Hard coal
   { "B06",  650, GenCategory::FOSSIL    },   // Fossil Oil
   { "B07",  900, GenCategory::FOSSIL    },   // Fossil Oil shale
   { "B08", 1000, GenCategory::FOSSIL    },   // Fossil Peat
   { "B09",   38, GenCategory::RENEWABLE },   // Geothermal
   { "B10",    0, GenCategory::STORAGE   },   // Hydro Pumped Storage
   { "B11",   24, GenCategory::RENEWABLE },   // Hydro Run-of-river and poundage
   { "B12",   24, GenCategory::RENEWABLE },   // Hydro Water Reservoir
   { "B13",   17, GenCategory::RENEWABLE },   // Marine
   { "B14",   12, GenCategory::NUCLEAR   },   // Nuclear
   { "B15",  230, GenCategory::RENEWABLE },   // Other renewable
   { "B16",   45, GenCategory::RENEWABLE },   // Solar
   { "B17",  700, GenCategory::FOSSIL    },   // Waste
   { "B18",   12, GenCategory::RENEWABLE },   // Wind Offshore
   { "B19",   11, GenCategory::RENEWABLE },   // Wind Onshore
   { "B20",  500, GenCategory::OTHER     }    // Other
};

/**
 * Friendly names for production types.
 */
struct PsrTypeName
{
   const char *code;
   const TCHAR *name;
};

static const PsrTypeName s_psrTypeNames[] =
{
   { "B01", _T("Biomass") },
   { "B02", _T("Fossil Brown coal/Lignite") },
   { "B03", _T("Fossil Coal-derived gas") },
   { "B04", _T("Fossil Gas") },
   { "B05", _T("Fossil Hard coal") },
   { "B06", _T("Fossil Oil") },
   { "B07", _T("Fossil Oil shale") },
   { "B08", _T("Fossil Peat") },
   { "B09", _T("Geothermal") },
   { "B10", _T("Hydro Pumped Storage") },
   { "B11", _T("Hydro Run-of-river and poundage") },
   { "B12", _T("Hydro Water Reservoir") },
   { "B13", _T("Marine") },
   { "B14", _T("Nuclear") },
   { "B15", _T("Other renewable") },
   { "B16", _T("Solar") },
   { "B17", _T("Waste") },
   { "B18", _T("Wind Offshore") },
   { "B19", _T("Wind Onshore") },
   { "B20", _T("Other") }
};

/**
 * Codes for which an "unmapped" warning has already been emitted.
 * Accessed only from the single poller thread.
 */
static StringSet s_warnedCodes;

/**
 * Get friendly name for a production type code.
 */
const TCHAR *PsrTypeFriendlyName(const char *code)
{
   for(size_t i = 0; i < sizeof(s_psrTypeNames) / sizeof(PsrTypeName); i++)
      if (!strcmp(s_psrTypeNames[i].code, code))
         return s_psrTypeNames[i].name;
   return _T("Unknown");
}

/**
 * Category display name.
 */
const TCHAR *CategoryName(GenCategory c)
{
   switch(c)
   {
      case GenCategory::RENEWABLE: return _T("renewable");
      case GenCategory::NUCLEAR:   return _T("nuclear");
      case GenCategory::FOSSIL:    return _T("fossil");
      case GenCategory::STORAGE:   return _T("storage");
      default:                     return _T("other");
   }
}

/**
 * Parse category name from configuration file.
 */
static GenCategory ParseCategory(const TCHAR *s)
{
   if (!_tcsicmp(s, _T("renewable")))
      return GenCategory::RENEWABLE;
   if (!_tcsicmp(s, _T("nuclear")))
      return GenCategory::NUCLEAR;
   if (!_tcsicmp(s, _T("fossil")))
      return GenCategory::FOSSIL;
   if (!_tcsicmp(s, _T("storage")))
      return GenCategory::STORAGE;
   return GenCategory::OTHER;
}

/**
 * Constructor
 */
EmissionFactorSet::EmissionFactorSet() : m_factors(32, 32)
{
   m_defaultFactor = 500;
   loadDefaults();
}

/**
 * Load compiled-in default factors.
 */
void EmissionFactorSet::loadDefaults()
{
   m_factors.clear();
   for(size_t i = 0; i < sizeof(s_defaultFactors) / sizeof(EmissionFactor); i++)
      m_factors.add(&s_defaultFactors[i]);
}

/**
 * Find factor entry for a production type code.
 */
const EmissionFactor *EmissionFactorSet::find(const char *code) const
{
   for(int i = 0; i < m_factors.size(); i++)
      if (!strcmp(m_factors.get(i)->code, code))
         return m_factors.get(i);
   return nullptr;
}

/**
 * Set or replace a factor entry.
 */
static void SetFactor(StructArray<EmissionFactor> *factors, const char *code, double grams, GenCategory category)
{
   for(int i = 0; i < factors->size(); i++)
   {
      if (!strcmp(factors->get(i)->code, code))
      {
         factors->get(i)->gramsPerKWh = grams;
         factors->get(i)->category = category;
         return;
      }
   }
   EmissionFactor f;
   strlcpy(f.code, code, MAX_PSR_TYPE);
   f.gramsPerKWh = grams;
   f.category = category;
   factors->add(&f);
}

/**
 * Load emission factor overrides from a file. Format, one entry per line:
 *   B04 = 490, fossil
 * Lines starting with '#' and blank lines are ignored.
 */
bool EmissionFactorSet::loadFromFile(const TCHAR *path)
{
   FILE *f = _tfopen(path, _T("r"));
   if (f == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open emission factor file %s"), path);
      return false;
   }

   int count = 0;
   char line[256];
   while(fgets(line, sizeof(line), f) != nullptr)
   {
      char *p = line;
      while((*p == ' ') || (*p == '\t'))
         p++;
      if ((*p == '#') || (*p == 0) || (*p == '\r') || (*p == '\n'))
         continue;

      // code = grams, category
      char *eq = strchr(p, '=');
      if (eq == nullptr)
         continue;
      *eq = 0;
      char code[MAX_PSR_TYPE];
      strlcpy(code, p, MAX_PSR_TYPE);
      TrimA(code);

      char *rest = eq + 1;
      char *comma = strchr(rest, ',');
      GenCategory category = GenCategory::OTHER;
      if (comma != nullptr)
      {
         *comma = 0;
         char catStr[32];
         strlcpy(catStr, comma + 1, sizeof(catStr));
         TrimA(catStr);
#ifdef UNICODE
         WCHAR catW[32];
         utf8_to_wchar(catStr, -1, catW, 32);
         category = ParseCategory(catW);
#else
         category = ParseCategory(catStr);
#endif
      }
      double grams = strtod(rest, nullptr);
      SetFactor(&m_factors, code, grams, category);
      count++;
   }
   fclose(f);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Loaded %d emission factor overrides from %s"), count, path);
   return true;
}

/**
 * Compute carbon intensity and shares for a generation snapshot.
 * Storage discharge is excluded from all denominators; unmapped production
 * types use the configured default factor and trigger a one-time warning.
 */
void ComputeCarbonMetrics(GenerationSnapshot *snapshot, const EmissionFactorSet& factors)
{
   double total = 0, weighted = 0, renewable = 0, lowCarbon = 0;
   for(int i = 0; i < snapshot->byType.size(); i++)
   {
      GenerationValue *g = snapshot->byType.get(i);
      const EmissionFactor *f = factors.find(g->psrType);
      if (f != nullptr)
      {
         g->factor = f->gramsPerKWh;
         g->category = f->category;
      }
      else
      {
         g->factor = factors.defaultFactor();
         g->category = GenCategory::OTHER;
#ifdef UNICODE
         WCHAR codeW[MAX_PSR_TYPE];
         utf8_to_wchar(g->psrType, -1, codeW, MAX_PSR_TYPE);
#else
         const char *codeW = g->psrType;
#endif
         if (!s_warnedCodes.contains(codeW))
         {
            s_warnedCodes.add(codeW);
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
               _T("Production type %hs has no emission factor mapping; using default %f gCO2/kWh"),
               g->psrType, factors.defaultFactor());
         }
      }

      if (g->category == GenCategory::STORAGE)
         continue;
      if (g->mw < 0)
         continue;

      total += g->mw;
      weighted += g->mw * g->factor;
      if (g->category == GenCategory::RENEWABLE)
      {
         renewable += g->mw;
         lowCarbon += g->mw;
      }
      else if (g->category == GenCategory::NUCLEAR)
      {
         lowCarbon += g->mw;
      }
   }

   snapshot->totalMW = total;
   snapshot->carbonIntensity = (total > 0) ? weighted / total : 0;
   snapshot->renewableShare = (total > 0) ? renewable / total * 100.0 : 0;
   snapshot->lowCarbonShare = (total > 0) ? lowCarbon / total * 100.0 : 0;
}
