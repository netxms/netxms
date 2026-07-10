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
** File: adapter.cpp
** ENTSO-E API client and document parsing (the only ENTSO-E-aware code).
**
**/

#include "entsoe.h"
#include <netxms-version.h>
#include <nxlibcurl.h>
#include <pugixml.h>
#include <math.h>

/**
 * Parse ISO 8601 UTC timestamp as used by ENTSO-E (e.g. "2026-07-09T10:00Z"
 * or "2026-07-09T10:00:00Z"). Returns 0 on failure.
 */
time_t ParseIso8601Utc(const char *str)
{
   if ((str == nullptr) || (*str == 0))
      return 0;
   int y, mo, d, h, mi, s = 0;
   int n = sscanf(str, "%d-%d-%dT%d:%d:%dZ", &y, &mo, &d, &h, &mi, &s);
   if (n < 5)
      return 0;
   struct tm t;
   memset(&t, 0, sizeof(t));
   t.tm_year = y - 1900;
   t.tm_mon = mo - 1;
   t.tm_mday = d;
   t.tm_hour = h;
   t.tm_min = mi;
   t.tm_sec = s;
   return timegm(&t);
}

/**
 * Parse ISO 8601 duration used for ENTSO-E resolution (PT15M, PT60M, PT30M,
 * PT1H, P1D). Returns duration in seconds, or 0 on failure.
 */
int ParseIso8601Duration(const char *str)
{
   if ((str == nullptr) || (str[0] != 'P'))
      return 0;
   int total = 0;
   bool inTime = false;
   for(const char *p = str + 1; *p != 0; )
   {
      if (*p == 'T')
      {
         inTime = true;
         p++;
         continue;
      }
      int num = 0;
      bool haveDigit = false;
      while(isdigit(static_cast<unsigned char>(*p)))
      {
         num = num * 10 + (*p - '0');
         p++;
         haveDigit = true;
      }
      if (!haveDigit || (*p == 0))
         break;
      switch(*p)
      {
         case 'D': total += num * 86400; break;
         case 'H': total += num * 3600; break;
         case 'M': total += inTime ? num * 60 : num * 2592000; break;
         case 'S': total += num; break;
         default: break;
      }
      p++;
   }
   return total;
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
bool EntsoeClient::get(const char *query, ByteStream *response, long *httpCode) const
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

   ByteStream *responseData = response;
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseData);

   char url[4096];
   snprintf(url, sizeof(url), "%s?securityToken=%s&%s", ENTSOE_API_URL, m_token, query);

   bool success = false;
   if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, httpCode);
         if ((*httpCode >= 200) && (*httpCode <= 299))
         {
            responseData->write('\0');
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("HTTP response code %03ld for query [%hs]"), *httpCode, query);
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
 * Raw data point within a period.
 */
struct RawPoint
{
   int position;
   double value;
};

/**
 * Parse a <Period> node into start time, resolution and points.
 */
static bool ParsePeriod(const pugi::xml_node& period, time_t *start, int *resolutionSec, StructArray<RawPoint> *points)
{
   pugi::xml_node ti = period.child("timeInterval");
   *start = ParseIso8601Utc(ti.child("start").child_value());
   if (*start == 0)
      return false;
   *resolutionSec = ParseIso8601Duration(period.child("resolution").child_value());
   if (*resolutionSec <= 0)
      return false;

   points->clear();
   for(pugi::xml_node pt = period.child("Point"); pt; pt = pt.next_sibling("Point"))
   {
      RawPoint rp;
      rp.position = atoi(pt.child("position").child_value());
      rp.value = atof(pt.child("quantity").child_value());
      if (rp.position >= 1)
         points->add(&rp);
   }
   return points->size() > 0;
}

/**
 * Value at position with forward-fill (ENTSO-E omits unchanged points, so a gap
 * means "same as the last present value"). Returns NaN if no point at or before.
 */
static double ValueAtForwardFill(const StructArray<RawPoint>& points, int pos)
{
   double v = NAN;
   for(int i = 0; i < points.size(); i++)
   {
      const RawPoint *p = points.get(i);
      if (p->position <= pos)
         v = p->value;
      else
         break;
   }
   return v;
}

/**
 * Latest fully-elapsed interval position for a period given current time.
 * Interval at position p covers [start+(p-1)*res, start+p*res); it is fully
 * elapsed when start+p*res <= now. Returns 0 if none elapsed.
 */
static int LatestElapsedPosition(time_t start, int resolutionSec, time_t now)
{
   if (now <= start)
      return 0;
   return static_cast<int>((now - start) / resolutionSec);
}

/**
 * Detect and log an ENTSO-E acknowledgement (error) document. Returns true if
 * the document is an acknowledgement.
 */
static bool IsAcknowledgement(const pugi::xml_node& root)
{
   if (strcmp(root.name(), "Acknowledgement_MarketDocument") != 0)
      return false;
   const char *reason = root.child("Reason").child("text").child_value();
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ENTSO-E acknowledgement document: %hs"),
      ((reason != nullptr) && (*reason != 0)) ? reason : "(no reason text)");
   return true;
}

/**
 * Accumulator for one production type.
 */
struct GenAcc
{
   char psr[MAX_PSR_TYPE];
   double mw;
   time_t t;
};

/**
 * Parse an actual-generation-per-type (A75) document.
 */
GenerationSnapshot *ParseGenerationDocument(const char *xml, size_t len, const EmissionFactorSet& factors)
{
   pugi::xml_document doc;
   pugi::xml_parse_result result = doc.load_buffer(xml, len);
   if (!result)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot parse generation document (%hs)"), result.description());
      return nullptr;
   }

   pugi::xml_node root = doc.first_child();
   if (IsAcknowledgement(root))
      return nullptr;

   time_t now = time(nullptr);
   StructArray<GenAcc> accum(24, 24);
   time_t snapTime = 0;

   for(pugi::xml_node ts = root.child("TimeSeries"); ts; ts = ts.next_sibling("TimeSeries"))
   {
      // Skip consumption/withdrawal series (e.g. pumped-storage consumption),
      // which carry outBiddingZone_Domain but not inBiddingZone_Domain.
      bool hasIn = ts.child("inBiddingZone_Domain.mRID").child_value()[0] != 0;
      bool hasOut = ts.child("outBiddingZone_Domain.mRID").child_value()[0] != 0;
      if (!hasIn && hasOut)
         continue;

      const char *psr = ts.child("MktPSRType").child("psrType").child_value();
      if ((psr == nullptr) || (*psr == 0))
         continue;

      pugi::xml_node period = ts.child("Period");
      time_t start;
      int resolutionSec;
      StructArray<RawPoint> points(96, 96);
      if (!ParsePeriod(period, &start, &resolutionSec, &points))
         continue;

      int pos = LatestElapsedPosition(start, resolutionSec, now);
      if (pos < 1)
         continue;
      double v = ValueAtForwardFill(points, pos);
      if (isnan(v))
         continue;
      time_t t = start + static_cast<time_t>(pos - 1) * resolutionSec;

      // Accumulate by production type, keeping the value from the latest interval.
      GenAcc *found = nullptr;
      for(int i = 0; i < accum.size(); i++)
      {
         if (!strcmp(accum.get(i)->psr, psr))
         {
            found = accum.get(i);
            break;
         }
      }
      if (found == nullptr)
      {
         GenAcc a;
         strlcpy(a.psr, psr, MAX_PSR_TYPE);
         a.mw = v;
         a.t = t;
         accum.add(&a);
      }
      else if (t > found->t)
      {
         found->mw = v;
         found->t = t;
      }
      else if (t == found->t)
      {
         found->mw += v;
      }
      if (t > snapTime)
         snapTime = t;
   }

   if (accum.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Generation document contains no usable data"));
      return nullptr;
   }

   GenerationSnapshot *snapshot = new GenerationSnapshot();
   snapshot->intervalTime = snapTime;
   for(int i = 0; i < accum.size(); i++)
   {
      GenerationValue gv;
      strlcpy(gv.psrType, accum.get(i)->psr, MAX_PSR_TYPE);
      gv.mw = accum.get(i)->mw;
      gv.category = GenCategory::OTHER;
      gv.factor = 0;
      snapshot->byType.add(&gv);
   }
   ComputeCarbonMetrics(snapshot, factors);
   return snapshot;
}

/**
 * Find or create a forecast point for a given target time.
 */
static ForecastPoint *FindOrCreateForecastPoint(ForecastCurve *curve, time_t targetTime)
{
   for(int i = 0; i < curve->points.size(); i++)
      if (curve->points.get(i)->targetTime == targetTime)
         return curve->points.get(i);
   ForecastPoint p;
   p.targetTime = targetTime;
   p.windMW = NAN;
   p.solarMW = NAN;
   p.loadMW = NAN;
   curve->points.add(&p);
   return curve->points.get(curve->points.size() - 1);
}

/**
 * Parse a forecast document (A69 wind/solar or A65 load) and merge into the
 * forecast curve. All points from the document are kept (day-ahead horizon).
 */
bool ParseForecastDocument(const char *xml, size_t len, ForecastCurve *curve, int field)
{
   pugi::xml_document doc;
   if (!doc.load_buffer(xml, len))
      return false;

   pugi::xml_node root = doc.first_child();
   if (IsAcknowledgement(root))
      return false;

   bool any = false;
   for(pugi::xml_node ts = root.child("TimeSeries"); ts; ts = ts.next_sibling("TimeSeries"))
   {
      const char *psr = ts.child("MktPSRType").child("psrType").child_value();
      pugi::xml_node period = ts.child("Period");
      time_t start;
      int resolutionSec;
      StructArray<RawPoint> points(96, 96);
      if (!ParsePeriod(period, &start, &resolutionSec, &points))
         continue;

      for(int i = 0; i < points.size(); i++)
      {
         const RawPoint *rp = points.get(i);
         time_t targetTime = start + static_cast<time_t>(rp->position - 1) * resolutionSec;
         ForecastPoint *fp = FindOrCreateForecastPoint(curve, targetTime);
         if (field == FORECAST_LOAD)
         {
            fp->loadMW = rp->value;
         }
         else   // FORECAST_WINDSOLAR
         {
            if (!strcmp(psr, "B16"))
               fp->solarMW = isnan(fp->solarMW) ? rp->value : fp->solarMW + rp->value;
            else if (!strcmp(psr, "B18") || !strcmp(psr, "B19"))
               fp->windMW = isnan(fp->windMW) ? rp->value : fp->windMW + rp->value;
         }
         any = true;
      }
   }
   return any;
}

/**
 * Parse a cross-border physical flow (A11) document and return the value of the
 * latest fully-elapsed interval. Sets ok to false on failure/no data.
 */
double ParseLatestFlow(const char *xml, size_t len, bool *ok)
{
   *ok = false;
   pugi::xml_document doc;
   if (!doc.load_buffer(xml, len))
      return 0;

   pugi::xml_node root = doc.first_child();
   if (IsAcknowledgement(root))
      return 0;

   time_t now = time(nullptr);
   double value = 0;
   time_t best = 0;
   for(pugi::xml_node ts = root.child("TimeSeries"); ts; ts = ts.next_sibling("TimeSeries"))
   {
      pugi::xml_node period = ts.child("Period");
      time_t start;
      int resolutionSec;
      StructArray<RawPoint> points(96, 96);
      if (!ParsePeriod(period, &start, &resolutionSec, &points))
         continue;
      int pos = LatestElapsedPosition(start, resolutionSec, now);
      if (pos < 1)
         continue;
      double v = ValueAtForwardFill(points, pos);
      if (isnan(v))
         continue;
      time_t t = start + static_cast<time_t>(pos - 1) * resolutionSec;
      if (t >= best)
      {
         best = t;
         value = v;
         *ok = true;
      }
   }
   return value;
}
