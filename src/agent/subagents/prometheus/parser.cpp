/*
** NetXMS Prometheus subagent
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
**/

#include "prometheus.h"

/**
 * Check if character is valid within metric or label name
 */
static inline bool IsNameChar(char c)
{
   return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_') || (c == ':') || (c == '.');
}

/**
 * Skip spaces and tabs
 */
static inline const char *SkipWhitespace(const char *p, const char *end)
{
   while((p < end) && ((*p == ' ') || (*p == '\t')))
      p++;
   return p;
}

/**
 * Parse single sample line in text exposition format:
 *    metric_name [ '{' label '=' '"' value '"' [ ',' ... ] '}' ] value [ timestamp ]
 * Returns nullptr if line cannot be parsed.
 */
static MetricSample *ParseSampleLine(const char *line, const char *end)
{
   const char *p = line;

   const char *nameStart = p;
   while((p < end) && IsNameChar(*p))
      p++;
   if (p == nameStart)
      return nullptr;

   MetricSample *sample = new MetricSample(nameStart, p - nameStart);

   if ((p < end) && (*p == '{'))
   {
      p++;
      while(true)
      {
         p = SkipWhitespace(p, end);
         if (p >= end)
         {
            delete sample;
            return nullptr;
         }
         if (*p == '}')
         {
            p++;
            break;
         }

         const char *labelStart = p;
         while((p < end) && IsNameChar(*p))
            p++;
         size_t labelLen = p - labelStart;
         p = SkipWhitespace(p, end);
         if ((p >= end) || (*p != '=') || (labelLen == 0))
         {
            delete sample;
            return nullptr;
         }
         p = SkipWhitespace(p + 1, end);
         if ((p >= end) || (*p != '"'))
         {
            delete sample;
            return nullptr;
         }
         p++;

         const char *valueStart = p;
         while((p < end) && (*p != '"'))
         {
            if ((*p == '\\') && (p + 1 < end))
               p++;
            p++;
         }
         if (p >= end)
         {
            delete sample;
            return nullptr;
         }

         char *value = MemAllocStringA(p - valueStart + 1);
         char *out = value;
         for(const char *s = valueStart; s < p; s++)
         {
            if ((*s == '\\') && (s + 1 < p))
            {
               s++;
               if (*s == 'n')
               {
                  *out++ = '\n';
               }
               else if ((*s == '\\') || (*s == '"'))
               {
                  *out++ = *s;
               }
               else
               {
                  *out++ = '\\';
                  *out++ = *s;
               }
            }
            else
            {
               *out++ = *s;
            }
         }
         *out = 0;
         sample->addLabel(labelStart, labelLen, value);
         p++;   // skip closing quote

         p = SkipWhitespace(p, end);
         if ((p < end) && (*p == ','))
         {
            p++;
         }
         else if ((p < end) && (*p != '}'))
         {
            delete sample;
            return nullptr;
         }
      }
   }

   p = SkipWhitespace(p, end);
   if (p >= end)
   {
      delete sample;
      return nullptr;
   }

   // Copy value token into temporary buffer for strtod (line is not null-terminated);
   // strtod handles special values NaN, +Inf, -Inf
   char token[64];
   size_t tokenLen = 0;
   while((p < end) && (*p != ' ') && (*p != '\t') && (tokenLen < sizeof(token) - 1))
      token[tokenLen++] = *p++;
   token[tokenLen] = 0;
   if ((p < end) && (*p != ' ') && (*p != '\t'))
   {
      delete sample;   // value token too long to be valid, reject instead of parsing truncated prefix
      return nullptr;
   }

   char *eptr;
   double value = strtod(token, &eptr);
   if (eptr == token)
   {
      delete sample;
      return nullptr;
   }
   sample->setValue(value);

   // Remainder of the line (timestamp, OpenMetrics exemplar) is ignored
   return sample;
}

/**
 * Parse document in Prometheus text exposition format / OpenMetrics text format.
 * Comment, HELP, TYPE, and EOF lines are skipped; unparsable lines are logged and skipped.
 */
ObjectArray<MetricSample> *ParsePrometheusText(const char *data)
{
   auto samples = new ObjectArray<MetricSample>(256, 256, Ownership::True);
   const char *p = data;
   while(*p != 0)
   {
      const char *eol = strchr(p, '\n');
      const char *lineEnd = (eol != nullptr) ? eol : p + strlen(p);
      if ((lineEnd > p) && (*(lineEnd - 1) == '\r'))
         lineEnd--;

      const char *s = SkipWhitespace(p, lineEnd);
      if ((s < lineEnd) && (*s != '#'))
      {
         MetricSample *sample = ParseSampleLine(s, lineEnd);
         if (sample != nullptr)
         {
            samples->add(sample);
         }
         else
         {
            char fragment[121];
            size_t len = std::min(static_cast<size_t>(lineEnd - s), sizeof(fragment) - 1);
            memcpy(fragment, s, len);
            fragment[len] = 0;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot parse exposition format line \"%hs\""), fragment);
         }
      }

      if (eol == nullptr)
         break;
      p = eol + 1;
   }
   return samples;
}
