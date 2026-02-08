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
 * Helper: Read all lines from file into array
 */
static bool ReadFileLines(const char *path, StringList *lines, int maxLines = DEFAULT_MAX_LINES)
{
   FILE *f = fopen(path, "r");
   if (f == nullptr)
      return false;

   char buffer[65536];
   while (fgets(buffer, sizeof(buffer), f) != nullptr && lines->size() < maxLines)
   {
      // Remove trailing newline
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len - 1] == '\n')
         buffer[len - 1] = 0;
      if (len > 1 && buffer[len - 2] == '\r')
         buffer[len - 2] = 0;
      lines->addUTF8String(buffer);
   }

   fclose(f);
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
   const char *file = json_object_get_string_utf8(params, "file", nullptr);
   const char *pattern = json_object_get_string_utf8(params, "pattern", nullptr);
   if (file == nullptr || pattern == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameters 'file' and 'pattern' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int contextBefore = json_object_get_int32(params, "context_before", 0);
   int contextAfter = json_object_get_int32(params, "context_after", 0);
   int maxMatches = json_object_get_int32(params, "max_matches", 100);
   bool caseInsensitive = json_object_get_boolean(params, "case_insensitive", false);

   // Compile regex
   const char *errorMessage;
   pcre *re = CompilePattern(pattern, caseInsensitive, &errorMessage);
   if (re == nullptr)
   {
      SetError(result, "INVALID_PATTERN", "Failed to compile regular expression");
      return ERR_BAD_ARGUMENTS;
   }

   // Read file
   StringList lines;
   if (!ReadFileLines(file, &lines))
   {
      pcre_free(re);
      SetError(result, "FILE_ERROR", "Failed to open or read file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Find matches
   *result = json_object();
   json_t *matches = json_array();
   int matchCount = 0;

   for (int i = 0; i < lines.size() && matchCount < maxMatches; i++)
   {
      char *lineUtf8 = UTF8StringFromTString(lines.get(i));
      if (MatchPattern(re, lineUtf8))
      {
         json_t *match = json_object();
         json_object_set_new(match, "line_number", json_integer(i + 1));
         json_object_set_new(match, "content", json_string(lineUtf8));

         // Add context before
         if (contextBefore > 0)
         {
            json_t *before = json_array();
            int start = (i - contextBefore > 0) ? i - contextBefore : 0;
            for (int j = start; j < i; j++)
            {
               char *ctx = UTF8StringFromTString(lines.get(j));
               json_array_append_new(before, json_string(ctx));
               MemFree(ctx);
            }
            json_object_set_new(match, "context_before", before);
         }

         // Add context after
         if (contextAfter > 0)
         {
            json_t *after = json_array();
            int end = (i + contextAfter < lines.size()) ? i + contextAfter : lines.size() - 1;
            for (int j = i + 1; j <= end; j++)
            {
               char *ctx = UTF8StringFromTString(lines.get(j));
               json_array_append_new(after, json_string(ctx));
               MemFree(ctx);
            }
            json_object_set_new(match, "context_after", after);
         }

         json_array_append_new(matches, match);
         matchCount++;
      }
      MemFree(lineUtf8);
   }

   pcre_free(re);

   json_object_set_new(*result, "matches", matches);
   json_object_set_new(*result, "match_count", json_integer(matchCount));
   json_object_set_new(*result, "total_lines", json_integer(lines.size()));
   json_object_set_new(*result, "truncated", json_boolean(matchCount >= maxMatches));

   return ERR_SUCCESS;
}

/**
 * Tool: log_read
 */
uint32_t H_LogRead(json_t *params, json_t **result, AbstractCommSession *session)
{
   const char *file = json_object_get_string_utf8(params, "file", nullptr);
   if (file == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int startLine = json_object_get_int32(params, "start_line", 1);
   int endLine = json_object_get_int32(params, "end_line", startLine + 99);

   if (startLine < 1 || endLine < startLine)
   {
      SetError(result, "INVALID_RANGE", "Invalid line range specified");
      return ERR_BAD_ARGUMENTS;
   }

   FILE *f = fopen(file, "r");
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *lines = json_array();
   char buffer[65536];
   int lineNum = 0;
   int linesRead = 0;

   while (fgets(buffer, sizeof(buffer), f) != nullptr)
   {
      lineNum++;
      if (lineNum < startLine)
         continue;
      if (lineNum > endLine)
         break;

      // Remove trailing newline
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len - 1] == '\n')
         buffer[len - 1] = 0;
      if (len > 1 && buffer[len - 2] == '\r')
         buffer[len - 2] = 0;

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
   const char *file = json_object_get_string_utf8(params, "file", nullptr);
   if (file == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int numLines = json_object_get_int32(params, "lines", 100);
   if (numLines < 1)
      numLines = 100;
   if (numLines > DEFAULT_MAX_LINES)
      numLines = DEFAULT_MAX_LINES;

   StringList lines;
   if (!ReadFileLines(file, &lines))
   {
      SetError(result, "FILE_ERROR", "Failed to open or read file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *linesArray = json_array();

   int startIdx = (lines.size() > numLines) ? lines.size() - numLines : 0;
   for (int i = startIdx; i < lines.size(); i++)
   {
      json_t *lineObj = json_object();
      json_object_set_new(lineObj, "number", json_integer(i + 1));
      char *content = UTF8StringFromTString(lines.get(i));
      json_object_set_new(lineObj, "content", json_string(content));
      MemFree(content);
      json_array_append_new(linesArray, lineObj);
   }

   json_object_set_new(*result, "lines", linesArray);
   json_object_set_new(*result, "lines_returned", json_integer(lines.size() - startIdx));
   json_object_set_new(*result, "total_lines", json_integer(lines.size()));
   json_object_set_new(*result, "start_line", json_integer(startIdx + 1));

   return ERR_SUCCESS;
}

/**
 * Tool: log_head
 */
uint32_t H_LogHead(json_t *params, json_t **result, AbstractCommSession *session)
{
   const char *file = json_object_get_string_utf8(params, "file", nullptr);
   if (file == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int numLines = json_object_get_int32(params, "lines", 100);
   if (numLines < 1)
      numLines = 100;
   if (numLines > DEFAULT_MAX_LINES)
      numLines = DEFAULT_MAX_LINES;

   FILE *f = fopen(file, "r");
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *linesArray = json_array();
   char buffer[65536];
   int lineNum = 0;

   while (fgets(buffer, sizeof(buffer), f) != nullptr && lineNum < numLines)
   {
      lineNum++;

      // Remove trailing newline
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len - 1] == '\n')
         buffer[len - 1] = 0;
      if (len > 1 && buffer[len - 2] == '\r')
         buffer[len - 2] = 0;

      json_t *lineObj = json_object();
      json_object_set_new(lineObj, "number", json_integer(lineNum));
      json_object_set_new(lineObj, "content", json_string(buffer));
      json_array_append_new(linesArray, lineObj);
   }

   // Check if there are more lines
   bool hasMore = (fgets(buffer, sizeof(buffer), f) != nullptr);
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
   const char *file = json_object_get_string_utf8(params, "file", nullptr);
   const char *pattern = json_object_get_string_utf8(params, "pattern", nullptr);
   if (file == nullptr || pattern == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameters 'file' and 'pattern' must be provided");
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

   FILE *f = fopen(file, "r");
   if (f == nullptr)
   {
      pcre_free(re);
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_t *lineNumbers = json_array();
   char buffer[65536];
   int lineNum = 0;
   int matchCount = 0;

   while (fgets(buffer, sizeof(buffer), f) != nullptr)
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
   String file = json_object_get_string(params, "file", _T(""));
   const char *startTimeStr = json_object_get_string_utf8(params, "start_time", nullptr);
   const char *endTimeStr = json_object_get_string_utf8(params, "end_time", nullptr);

   if (file.isEmpty() || startTimeStr == nullptr || endTimeStr == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameters 'file', 'start_time', and 'end_time' must be provided");
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
   char buffer[65536];
   int lineNum = 0;
   int linesReturned = 0;

   while (fgets(buffer, sizeof(buffer), f) != nullptr && linesReturned < maxLines)
   {
      lineNum++;

      // Try to extract timestamp
      char timestamp[64];
      if (MatchPatternWithCapture(re, buffer, timestamp, sizeof(timestamp)))
      {
         time_t lineTime = ParseTimestamp(timestamp);
         if (lineTime >= startTime && lineTime <= endTime)
         {
            // Remove trailing newline
            size_t bufLen = strlen(buffer);
            if (bufLen > 0 && buffer[bufLen - 1] == '\n')
               buffer[bufLen - 1] = 0;

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
   String file = json_object_get_string(params, "file", _T(""));
   json_t *patterns = json_object_get(params, "patterns");

   if (file.isEmpty() || patterns == nullptr || !json_is_array(patterns))
   {
      SetError(result, "MISSING_PARAM", "Required parameters 'file' and 'patterns' (array) must be provided");
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

   char buffer[65536];
   int64_t totalLines = 0;

   while (fgets(buffer, sizeof(buffer), f) != nullptr)
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
