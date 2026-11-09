/*
** NetXMS AI File Operations subagent
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: log.cpp
**
**/

#include "aifileops.h"
#include <netxms-regex.h>

/**
 * Line buffer size. Lines longer than this are truncated.
 */
#define LINE_BUFFER_SIZE   65536

/**
 * Helper: read next line from file into buffer and strip line terminator. Remainder of
 * a line that does not fit into the buffer is discarded, so each call consumes exactly
 * one line and line numbers stay consistent across all log tools. Returns false at end of file.
 */
static bool ReadLine(FILE *f, char *buffer, size_t size)
{
   if (fgets(buffer, static_cast<int>(size), f) == nullptr)
      return false;

   size_t len = strlen(buffer);
   if ((len > 0) && (buffer[len - 1] == '\n'))
   {
      buffer[--len] = 0;
      if ((len > 0) && (buffer[len - 1] == '\r'))
         buffer[--len] = 0;
   }
   else if (len == size - 1)
   {
      char discard[1024];
      while (fgets(discard, sizeof(discard), f) != nullptr)
      {
         size_t l = strlen(discard);
         if ((l > 0) && (discard[l - 1] == '\n'))
            break;
      }
   }
   return true;
}

/**
 * Helper: Compile PCRE pattern
 */
static pcre *CompilePattern(const char *pattern, bool caseInsensitive, const char **errorMessage)
{
   int erroffset;
   int options = PCRE_COMMON_FLAGS_A;
   if (caseInsensitive)
      options |= PCRE_CASELESS;

   pcre *re = pcre_compile(pattern, options, errorMessage, &erroffset, nullptr);
   return re;
}

/**
 * Helper: Match pattern against string
 */
static bool MatchPattern(pcre *re, const char *str)
{
   int ovector[30];
   int rc = pcre_exec(re, nullptr, str, (int)strlen(str), 0, 0, ovector, 30);
   return rc >= 0;
}

/**
 * Helper: Match with capture groups
 */
static bool MatchPatternWithCapture(pcre *re, const char *str, char *capture, size_t captureSize)
{
   int ovector[30];
   int rc = pcre_exec(re, nullptr, str, (int)strlen(str), 0, 0, ovector, 30);
   if (rc >= 2)
   {
      int start = ovector[2];
      int end = ovector[3];
      int len = end - start;
      if (len > 0 && (size_t)len < captureSize)
      {
         memcpy(capture, str + start, len);
         capture[len] = 0;
         return true;
      }
   }
   return false;
}

/**
 * Tool: log_grep
 */
uint32_t H_LogGrep(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   const char *pattern = json_object_get_string_utf8(params, "pattern", nullptr);
   if (pattern == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'pattern' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int contextBefore = json_object_get_int32(params, "context_before", 0);
   int contextAfter = json_object_get_int32(params, "context_after", 0);
   int maxMatches = json_object_get_int32(params, "max_matches", 100);
   bool caseInsensitive = json_object_get_boolean(params, "case_insensitive", false);

   if (contextBefore < 0)
      contextBefore = 0;
   else if (contextBefore > MAX_CONTEXT_LINES)
      contextBefore = MAX_CONTEXT_LINES;
   if (contextAfter < 0)
      contextAfter = 0;
   else if (contextAfter > MAX_CONTEXT_LINES)
      contextAfter = MAX_CONTEXT_LINES;
   if (maxMatches < 1)
      maxMatches = 1;
   else if (maxMatches > DEFAULT_MAX_MATCHES)
      maxMatches = DEFAULT_MAX_MATCHES;

   // Compile regex
   const char *errorMessage;
   pcre *re = CompilePattern(pattern, caseInsensitive, &errorMessage);
   if (re == nullptr)
   {
      SetError(result, "INVALID_PATTERN", "Failed to compile regular expression");
      return ERR_BAD_ARGUMENTS;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      pcre_free(re);
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Ring buffer of preceding lines for context_before; line N is stored at index (N - 1) % contextBefore
   char **history = (contextBefore > 0) ? MemAllocArray<char*>(contextBefore) : nullptr;

   *result = json_object();
   json_t *matches = json_array();
   char buffer[LINE_BUFFER_SIZE];
   int64_t lineNum = 0;
   int matchCount = 0;
   int pendingAfter = 0;   // Number of lines still needed as context_after for recent matches
   bool truncated = false;

   while (ReadLine(f, buffer, sizeof(buffer)))
   {
      lineNum++;

      // Match limit reached and trailing context collected - stop without scanning rest of the file
      if ((matchCount >= maxMatches) && (pendingAfter == 0))
      {
         truncated = true;
         break;
      }

      // Add this line as context_after to all recent matches that are close enough
      if (pendingAfter > 0)
      {
         for (size_t i = json_array_size(matches); i > 0; i--)
         {
            json_t *m = json_array_get(matches, i - 1);
            if (json_integer_value(json_object_get(m, "line_number")) < lineNum - contextAfter)
               break;
            json_array_append_new(json_object_get(m, "context_after"), json_string(buffer));
         }
         pendingAfter--;
      }

      if ((matchCount < maxMatches) && MatchPattern(re, buffer))
      {
         json_t *match = json_object();
         json_object_set_new(match, "line_number", json_integer(lineNum));
         json_object_set_new(match, "content", json_string(buffer));

         if (contextBefore > 0)
         {
            json_t *before = json_array();
            for (int64_t j = std::max(lineNum - contextBefore, static_cast<int64_t>(1)); j < lineNum; j++)
               json_array_append_new(before, json_string(history[(j - 1) % contextBefore]));
            json_object_set_new(match, "context_before", before);
         }

         if (contextAfter > 0)
         {
            json_object_set_new(match, "context_after", json_array());
            pendingAfter = contextAfter;
         }

         json_array_append_new(matches, match);
         matchCount++;
      }

      if (contextBefore > 0)
      {
         int slot = static_cast<int>((lineNum - 1) % contextBefore);
         MemFree(history[slot]);
         history[slot] = MemCopyStringA(buffer);
      }
   }

   fclose(f);
   pcre_free(re);
   if (history != nullptr)
   {
      for (int i = 0; i < contextBefore; i++)
         MemFree(history[i]);
      MemFree(history);
   }

   json_object_set_new(*result, "matches", matches);
   json_object_set_new(*result, "match_count", json_integer(matchCount));
   json_object_set_new(*result, "total_lines_scanned", json_integer(truncated ? lineNum - 1 : lineNum));
   json_object_set_new(*result, "truncated", json_boolean(truncated));

   return ERR_SUCCESS;
}

/**
 * Tool: log_read
 */
uint32_t H_LogRead(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   int startLine = json_object_get_int32(params, "start_line", 1);
   int endLine = json_object_get_int32(params, "end_line", startLine + 99);

   if (startLine < 1 || endLine < startLine)
   {
      SetError(result, "INVALID_RANGE", "Invalid line range specified");
      return ERR_BAD_ARGUMENTS;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *lines = json_array();
   char buffer[LINE_BUFFER_SIZE];
   int lineNum = 0;
   int linesRead = 0;

   while (ReadLine(f, buffer, sizeof(buffer)))
   {
      lineNum++;
      if (lineNum < startLine)
         continue;
      if (lineNum > endLine)
         break;

      json_t *lineObj = json_object();
      json_object_set_new(lineObj, "number", json_integer(lineNum));
      json_object_set_new(lineObj, "content", json_string(buffer));
      json_array_append_new(lines, lineObj);
      linesRead++;
   }

   fclose(f);

   json_object_set_new(*result, "lines", lines);
   json_object_set_new(*result, "lines_read", json_integer(linesRead));
   json_object_set_new(*result, "start_line", json_integer(startLine));
   json_object_set_new(*result, "end_line", json_integer(startLine + linesRead - 1));

   return ERR_SUCCESS;
}

/**
 * Tool: log_tail
 */
uint32_t H_LogTail(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   int numLines = json_object_get_int32(params, "lines", 100);
   if (numLines < 1)
      numLines = 100;
   if (numLines > DEFAULT_MAX_LINES)
      numLines = DEFAULT_MAX_LINES;

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   // First pass: count lines
   char buffer[LINE_BUFFER_SIZE];
   int64_t totalLines = 0;
   while (ReadLine(f, buffer, sizeof(buffer)))
      totalLines++;

   // Second pass: skip to the first requested line and return the rest
   rewind(f);
   int64_t skip = (totalLines > numLines) ? totalLines - numLines : 0;
   for (int64_t i = 0; (i < skip) && ReadLine(f, buffer, sizeof(buffer)); i++)
      ;

   *result = json_object();
   json_t *linesArray = json_array();
   int linesReturned = 0;
   while ((linesReturned < numLines) && ReadLine(f, buffer, sizeof(buffer)))
   {
      json_t *lineObj = json_object();
      json_object_set_new(lineObj, "number", json_integer(skip + linesReturned + 1));
      json_object_set_new(lineObj, "content", json_string(buffer));
      json_array_append_new(linesArray, lineObj);
      linesReturned++;
   }

   fclose(f);

   json_object_set_new(*result, "lines", linesArray);
   json_object_set_new(*result, "lines_returned", json_integer(linesReturned));
   json_object_set_new(*result, "total_lines", json_integer(totalLines));
   json_object_set_new(*result, "start_line", json_integer(skip + 1));

   return ERR_SUCCESS;
}

/**
 * Tool: log_head
 */
uint32_t H_LogHead(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   int numLines = json_object_get_int32(params, "lines", 100);
   if (numLines < 1)
      numLines = 100;
   if (numLines > DEFAULT_MAX_LINES)
      numLines = DEFAULT_MAX_LINES;

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *linesArray = json_array();
   char buffer[LINE_BUFFER_SIZE];
   int lineNum = 0;

   while ((lineNum < numLines) && ReadLine(f, buffer, sizeof(buffer)))
   {
      lineNum++;

      json_t *lineObj = json_object();
      json_object_set_new(lineObj, "number", json_integer(lineNum));
      json_object_set_new(lineObj, "content", json_string(buffer));
      json_array_append_new(linesArray, lineObj);
   }

   // Check if there are more lines
   bool hasMore = ReadLine(f, buffer, sizeof(buffer));
   fclose(f);

   json_object_set_new(*result, "lines", linesArray);
   json_object_set_new(*result, "lines_returned", json_integer(lineNum));
   json_object_set_new(*result, "has_more", json_boolean(hasMore));

   return ERR_SUCCESS;
}

/**
 * Tool: log_find
 */
uint32_t H_LogFind(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   const char *pattern = json_object_get_string_utf8(params, "pattern", nullptr);
   if (pattern == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'pattern' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int maxMatches = json_object_get_int32(params, "max_matches", 1000);
   bool caseInsensitive = json_object_get_boolean(params, "case_insensitive", false);

   // Compile regex
   const char *errorMessage;
   pcre *re = CompilePattern(pattern, caseInsensitive, &errorMessage);
   if (re == nullptr)
   {
      SetError(result, "INVALID_PATTERN", "Failed to compile regular expression");
      return ERR_BAD_ARGUMENTS;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      pcre_free(re);
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *lineNumbers = json_array();
   char buffer[LINE_BUFFER_SIZE];
   int lineNum = 0;
   int matchCount = 0;

   while (ReadLine(f, buffer, sizeof(buffer)))
   {
      lineNum++;
      if (MatchPattern(re, buffer))
      {
         json_array_append_new(lineNumbers, json_integer(lineNum));
         matchCount++;
         if (matchCount >= maxMatches)
            break;
      }
   }

   fclose(f);
   pcre_free(re);

   json_object_set_new(*result, "line_numbers", lineNumbers);
   json_object_set_new(*result, "match_count", json_integer(matchCount));
   json_object_set_new(*result, "total_lines_scanned", json_integer(lineNum));
   json_object_set_new(*result, "truncated", json_boolean(matchCount >= maxMatches));

   return ERR_SUCCESS;
}

/**
 * Tool: log_time_range
 */
uint32_t H_LogTimeRange(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   const char *startTimeStr = json_object_get_string_utf8(params, "start_time", nullptr);
   const char *endTimeStr = json_object_get_string_utf8(params, "end_time", nullptr);
   if (startTimeStr == nullptr || endTimeStr == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameters 'start_time' and 'end_time' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   time_t startTime = ParseTimestamp(startTimeStr);
   time_t endTime = ParseTimestamp(endTimeStr);

   if (startTime == 0 || endTime == 0)
   {
      SetError(result, "INVALID_TIME", "Failed to parse time values. Use ISO 8601 format (YYYY-MM-DDTHH:MM:SS)");
      return ERR_BAD_ARGUMENTS;
   }

   const char *tsPattern = json_object_get_string_utf8(params, "timestamp_pattern", "^(\\d{4}-\\d{2}-\\d{2}[T ]\\d{2}:\\d{2}:\\d{2})");
   int maxLines = json_object_get_int32(params, "max_lines", 1000);

   // Compile timestamp extraction regex
   const char *errorMessage;
   pcre *re = CompilePattern(tsPattern, false, &errorMessage);
   if (re == nullptr)
   {
      SetError(result, "INVALID_PATTERN", "Failed to compile timestamp pattern");
      return ERR_BAD_ARGUMENTS;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      pcre_free(re);
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *lines = json_array();
   char buffer[LINE_BUFFER_SIZE];
   int lineNum = 0;
   int linesReturned = 0;

   while ((linesReturned < maxLines) && ReadLine(f, buffer, sizeof(buffer)))
   {
      lineNum++;

      // Try to extract timestamp
      char timestamp[64];
      if (MatchPatternWithCapture(re, buffer, timestamp, sizeof(timestamp)))
      {
         time_t lineTime = ParseTimestamp(timestamp);
         if (lineTime >= startTime && lineTime <= endTime)
         {
            json_t *lineObj = json_object();
            json_object_set_new(lineObj, "number", json_integer(lineNum));
            json_object_set_new(lineObj, "content", json_string(buffer));
            json_object_set_new(lineObj, "timestamp", json_string(timestamp));
            json_array_append_new(lines, lineObj);
            linesReturned++;
         }
      }
   }

   fclose(f);
   pcre_free(re);

   json_object_set_new(*result, "lines", lines);
   json_object_set_new(*result, "lines_returned", json_integer(linesReturned));
   json_object_set_new(*result, "total_lines_scanned", json_integer(lineNum));
   json_object_set_new(*result, "truncated", json_boolean(linesReturned >= maxLines));

   return ERR_SUCCESS;
}

/**
 * Tool: log_stats
 */
uint32_t H_LogStats(json_t *params, json_t **result, AbstractCommSession *session)
{
   MutableString file;
   uint32_t rcc = ResolveToolPath(params, "file", false, result, &file);
   if (rcc != ERR_SUCCESS)
      return rcc;

   json_t *patterns = json_object_get(params, "patterns");
   if (patterns == nullptr || !json_is_array(patterns))
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'patterns' (array) must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   bool caseInsensitive = json_object_get_boolean(params, "case_insensitive", false);

   // Compile all patterns
   size_t numPatterns = json_array_size(patterns);
   struct PatternInfo
   {
      pcre *regex;
      const char *name;
      int64_t count;
   };

   PatternInfo *patternInfos = MemAllocArray<PatternInfo>(numPatterns);

   for (size_t i = 0; i < numPatterns; i++)
   {
      json_t *p = json_array_get(patterns, i);
      patternInfos[i].name = json_string_value(json_object_get(p, "name"));
      const char *pat = json_string_value(json_object_get(p, "pattern"));
      patternInfos[i].count = 0;
      patternInfos[i].regex = nullptr;

      if (pat != nullptr)
      {
         const char *errorMessage;
         patternInfos[i].regex = CompilePattern(pat, caseInsensitive, &errorMessage);
      }
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      for (size_t i = 0; i < numPatterns; i++)
         if (patternInfos[i].regex != nullptr)
            pcre_free(patternInfos[i].regex);
      MemFree(patternInfos);
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   char buffer[LINE_BUFFER_SIZE];
   int64_t totalLines = 0;

   while (ReadLine(f, buffer, sizeof(buffer)))
   {
      totalLines++;
      for (size_t i = 0; i < numPatterns; i++)
      {
         if (patternInfos[i].regex != nullptr && MatchPattern(patternInfos[i].regex, buffer))
         {
            patternInfos[i].count++;
         }
      }
   }

   fclose(f);

   // Build result
   *result = json_object();
   json_t *stats = json_array();

   for (size_t i = 0; i < numPatterns; i++)
   {
      json_t *stat = json_object();
      json_object_set_new(stat, "name", json_string(patternInfos[i].name ? patternInfos[i].name : "unnamed"));
      json_object_set_new(stat, "count", json_integer(patternInfos[i].count));
      if (totalLines > 0)
         json_object_set_new(stat, "percentage", json_real((double)patternInfos[i].count / totalLines * 100.0));
      json_array_append_new(stats, stat);

      if (patternInfos[i].regex != nullptr)
         pcre_free(patternInfos[i].regex);
   }

   MemFree(patternInfos);

   json_object_set_new(*result, "statistics", stats);
   json_object_set_new(*result, "total_lines", json_integer(totalLines));

   return ERR_SUCCESS;
}
