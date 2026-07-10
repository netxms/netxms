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
** File: catalog.cpp
** Built-in ENTSO-E bidding-zone catalog (name -> EIC + neighbours).
**
**/

#include "entsoe.h"
#include "catalog_data.h"

static ObjectArray<CatalogZone> s_catalog(64, 64, Ownership::True);

/**
 * Normalize a zone name for matching: uppercase, strip separators, so that
 * "SE4", "SE-4" and "SE_4" all match.
 */
static void NormalizeName(const char *name, char *out, size_t size)
{
   size_t j = 0;
   for(size_t i = 0; (name[i] != 0) && (j < size - 1); i++)
   {
      char c = name[i];
      if (isalnum(static_cast<unsigned char>(c)))
         out[j++] = toupper(static_cast<unsigned char>(c));
   }
   out[j] = 0;
}

/**
 * Find catalog entry by name (mutable).
 */
static CatalogZone *FindZone(const char *name)
{
   char norm[MAX_ZONE_NAME];
   NormalizeName(name, norm, sizeof(norm));
   for(int i = 0; i < s_catalog.size(); i++)
   {
      char n2[MAX_ZONE_NAME];
      NormalizeName(s_catalog.get(i)->name, n2, sizeof(n2));
      if (!strcmp(norm, n2))
         return s_catalog.get(i);
   }
   return nullptr;
}

/**
 * Create/replace a catalog entry.
 */
static void AddZone(const char *name, const char *eic, const char *neighbours)
{
   CatalogZone *z = FindZone(name);
   if (z == nullptr)
   {
      z = new CatalogZone();
      s_catalog.add(z);
   }
   else
   {
      z->neighbours.clear();
   }
   strlcpy(z->name, name, MAX_ZONE_NAME);
   strlcpy(z->eic, eic, MAX_EIC_LEN);
   if ((neighbours != nullptr) && (*neighbours != 0))
   {
      char buf[512];
      strlcpy(buf, neighbours, sizeof(buf));
      char *tok = buf;
      while(tok != nullptr)
      {
         char *comma = strchr(tok, ',');
         if (comma != nullptr)
            *comma = 0;
         TrimA(tok);
         if (*tok != 0)
         {
            TCHAR n[MAX_ZONE_NAME];
#ifdef UNICODE
            utf8_to_wchar(tok, -1, n, MAX_ZONE_NAME);
#else
            strlcpy(n, tok, MAX_ZONE_NAME);
#endif
            z->neighbours.add(n);
         }
         tok = (comma != nullptr) ? comma + 1 : nullptr;
      }
   }
}

/**
 * Initialize the built-in catalog.
 */
void InitZoneCatalog()
{
   s_catalog.clear();
   for(size_t i = 0; i < sizeof(s_builtinZones) / sizeof(BuiltinZone); i++)
      AddZone(s_builtinZones[i].name, s_builtinZones[i].eic, s_builtinZones[i].neighbours);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Built-in zone catalog initialized (%d zones)"), s_catalog.size());
}

/**
 * Load catalog overrides/additions from a file. One entry per line, CSV:
 *   NAME,EIC,neighbour1,neighbour2,...
 * Lines starting with '#' and blank lines are ignored.
 */
bool LoadZoneCatalogFile(const TCHAR *path)
{
   FILE *f = _tfopen(path, _T("r"));
   if (f == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open zone catalog file %s"), path);
      return false;
   }

   int count = 0;
   char line[512];
   while(fgets(line, sizeof(line), f) != nullptr)
   {
      char *p = line;
      while((*p == ' ') || (*p == '\t'))
         p++;
      if ((*p == '#') || (*p == 0) || (*p == '\r') || (*p == '\n'))
         continue;

      // Strip trailing EOL/whitespace
      char *end = p + strlen(p);
      while((end > p) && ((end[-1] == '\r') || (end[-1] == '\n') || (end[-1] == ' ') || (end[-1] == '\t')))
         *(--end) = 0;

      char *name = p;
      char *comma = strchr(p, ',');
      if (comma == nullptr)
         continue;
      *comma = 0;
      char *eic = comma + 1;
      char *neighbours = strchr(eic, ',');
      if (neighbours != nullptr)
      {
         *neighbours = 0;
         neighbours++;
      }
      TrimA(name);
      TrimA(eic);
      AddZone(name, eic, neighbours);
      count++;
   }
   fclose(f);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Loaded %d zone catalog entries from %s"), count, path);
   return true;
}

/**
 * Look up a catalog zone by name.
 */
const CatalogZone *LookupCatalogZone(const char *name)
{
   return FindZone(name);
}

/**
 * Look up a zone EIC code by name.
 */
const char *LookupZoneEic(const char *name)
{
   const CatalogZone *z = FindZone(name);
   return (z != nullptr) ? z->eic : nullptr;
}
