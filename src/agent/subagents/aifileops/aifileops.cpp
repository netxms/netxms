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
** File: aifileops.cpp
**
**/

#include "aifileops.h"
#include <netxms-version.h>
#include <nxstat.h>
#include <netxms-regex.h>

//
// Tool parameter definitions
//

static AIToolParameter s_logGrepParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "pattern", "string", "Regular expression pattern to search for", true, nullptr, nullptr },
   { "context_before", "integer", "Number of lines to include before each match", false, "0", "{\"minimum\":0,\"maximum\":100}" },
   { "context_after", "integer", "Number of lines to include after each match", false, "0", "{\"minimum\":0,\"maximum\":100}" },
   { "max_matches", "integer", "Maximum number of matches to return", false, "100", "{\"minimum\":1,\"maximum\":1000}" },
   { "case_insensitive", "boolean", "Perform case-insensitive matching", false, "false", nullptr }
};

static AIToolParameter s_logReadParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "start_line", "integer", "Starting line number (1-based)", true, nullptr, "{\"minimum\":1}" },
   { "end_line", "integer", "Ending line number (inclusive)", true, nullptr, "{\"minimum\":1}" }
};

static AIToolParameter s_logTailParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "lines", "integer", "Number of lines to read from end", false, "100", "{\"minimum\":1,\"maximum\":10000}" }
};

static AIToolParameter s_logHeadParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "lines", "integer", "Number of lines to read from beginning", false, "100", "{\"minimum\":1,\"maximum\":10000}" }
};

static AIToolParameter s_logFindParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "pattern", "string", "Regular expression pattern to search for", true, nullptr, nullptr },
   { "max_matches", "integer", "Maximum number of line numbers to return", false, "1000", "{\"minimum\":1,\"maximum\":10000}" },
   { "case_insensitive", "boolean", "Perform case-insensitive matching", false, "false", nullptr }
};

static AIToolParameter s_logTimeRangeParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "start_time", "string", "Start time in ISO 8601 format (YYYY-MM-DDTHH:MM:SS)", true, nullptr, nullptr },
   { "end_time", "string", "End time in ISO 8601 format (YYYY-MM-DDTHH:MM:SS)", true, nullptr, nullptr },
   { "timestamp_pattern", "string", "Regex pattern with capture group for timestamp extraction", false, "\"^(\\\\d{4}-\\\\d{2}-\\\\d{2}[T ]\\\\d{2}:\\\\d{2}:\\\\d{2})\"", nullptr },
   { "max_lines", "integer", "Maximum lines to return", false, "1000", "{\"minimum\":1,\"maximum\":10000}" }
};

static AIToolParameter s_logStatsParams[] =
{
   { "file", "string", "Path to log file", true, nullptr, nullptr },
   { "patterns", "array", "Array of pattern objects with 'name' and 'pattern' fields", true, nullptr, nullptr },
   { "case_insensitive", "boolean", "Perform case-insensitive matching", false, "false", nullptr }
};

static AIToolParameter s_fileInfoParams[] =
{
   { "file", "string", "Path to file", true, nullptr, nullptr },
   { "count_lines", "boolean", "Count number of lines (may be slow for large files)", false, "false", nullptr }
};

static AIToolParameter s_fileListParams[] =
{
   { "directory", "string", "Directory path to list", true, nullptr, nullptr },
   { "pattern", "string", "Glob pattern for filtering (e.g., *.log)", false, "\"*\"", nullptr },
   { "recursive", "boolean", "List files recursively", false, "false", nullptr },
   { "max_entries", "integer", "Maximum entries to return", false, "1000", "{\"minimum\":1,\"maximum\":10000}" }
};

static AIToolParameter s_fileReadParams[] =
{
   { "file", "string", "Path to file", true, nullptr, nullptr },
   { "offset", "integer", "Byte offset to start reading from", false, "0", "{\"minimum\":0}" },
   { "limit", "integer", "Maximum bytes to read", false, "65536", "{\"minimum\":1,\"maximum\":1048576}" }
};

//
// Tool definitions
//

static AIToolDefinition s_aiTools[] =
{
   {
      "log_grep",
      "log",
      "Search log file for lines matching a regular expression pattern. Returns matching lines with optional context. Use this to find specific errors, events, or patterns in log files.",
      s_logGrepParams,
      sizeof(s_logGrepParams) / sizeof(AIToolParameter),
      H_LogGrep
   },
   {
      "log_read",
      "log",
      "Read a specific range of lines from a log file by line numbers. Use this when you know exactly which lines you need, for example after using log_find to locate relevant line numbers.",
      s_logReadParams,
      sizeof(s_logReadParams) / sizeof(AIToolParameter),
      H_LogRead
   },
   {
      "log_tail",
      "log",
      "Read the last N lines from a log file. Use this to see recent log entries or check the current state of a log file.",
      s_logTailParams,
      sizeof(s_logTailParams) / sizeof(AIToolParameter),
      H_LogTail
   },
   {
      "log_head",
      "log",
      "Read the first N lines from a log file. Use this to see the beginning of a log file, check file format, or read header information.",
      s_logHeadParams,
      sizeof(s_logHeadParams) / sizeof(AIToolParameter),
      H_LogHead
   },
   {
      "log_find",
      "log",
      "Find line numbers where a pattern matches, without returning content. Use this to locate positions of matches in large files, then use log_read to retrieve specific sections.",
      s_logFindParams,
      sizeof(s_logFindParams) / sizeof(AIToolParameter),
      H_LogFind
   },
   {
      "log_time_range",
      "log",
      "Extract log entries within a specified time range. Use this to get all log entries between two timestamps.",
      s_logTimeRangeParams,
      sizeof(s_logTimeRangeParams) / sizeof(AIToolParameter),
      H_LogTimeRange
   },
   {
      "log_stats",
      "log",
      "Compute frequency statistics for patterns in a log file. Use this to count occurrences of errors, warnings, or other patterns to understand the distribution of events.",
      s_logStatsParams,
      sizeof(s_logStatsParams) / sizeof(AIToolParameter),
      H_LogStats
   },
   {
      "file_info",
      "file",
      "Get file metadata including size, modification time, permissions, and optionally line count. Use this to check if a file exists and get basic information before reading it.",
      s_fileInfoParams,
      sizeof(s_fileInfoParams) / sizeof(AIToolParameter),
      H_FileInfo
   },
   {
      "file_list",
      "file",
      "List directory contents with optional glob pattern filtering. Use this to discover available log files or explore directory structure.",
      s_fileListParams,
      sizeof(s_fileListParams) / sizeof(AIToolParameter),
      H_FileList
   },
   {
      "file_read",
      "file",
      "Read raw file contents with byte offset and limit. Use this for reading configuration files or any text file. For log files, prefer the specialized log_* tools.",
      s_fileReadParams,
      sizeof(s_fileReadParams) / sizeof(AIToolParameter),
      H_FileRead
   }
};

//
// Helper: Set JSON error response
//

static void SetError(json_t **result, const char *code, const char *message)
{
   *result = json_object();
   json_t *error = json_object();
   json_object_set_new(error, "code", json_string(code));
   json_object_set_new(error, "message", json_string(message));
   json_object_set_new(*result, "error", error);
}

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

/**
 * Tool: file_info
 */
uint32_t H_FileInfo(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   bool countLines = json_object_get_boolean(params, "count_lines", false);

   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      SetError(result, "FILE_ERROR", "Failed to stat file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "path", json_string_t(file));
   json_object_set_new(*result, "size", json_integer(st.st_size));
   json_object_set_new(*result, "mtime", json_integer(st.st_mtime));
   json_object_set_new(*result, "atime", json_integer(st.st_atime));
   json_object_set_new(*result, "ctime", json_integer(st.st_ctime));

#ifndef _WIN32
   json_object_set_new(*result, "mode", json_integer(st.st_mode & 0777));
   json_object_set_new(*result, "uid", json_integer(st.st_uid));
   json_object_set_new(*result, "gid", json_integer(st.st_gid));
#endif

   json_object_set_new(*result, "is_directory", json_boolean(S_ISDIR(st.st_mode)));
   json_object_set_new(*result, "is_regular_file", json_boolean(S_ISREG(st.st_mode)));
#ifndef _WIN32
   json_object_set_new(*result, "is_symlink", json_boolean(S_ISLNK(st.st_mode)));
#endif

   // Count lines if requested
   if (countLines && S_ISREG(st.st_mode))
   {
      FILE *f = _tfopen(file, _T("r"));
      if (f != nullptr)
      {
         int64_t lineCount = 0;
         int ch;
         while ((ch = fgetc(f)) != EOF)
         {
            if (ch == '\n')
               lineCount++;
         }
         fclose(f);
         json_object_set_new(*result, "line_count", json_integer(lineCount));
      }
   }

   return ERR_SUCCESS;
}

/**
 * Helper: Recursively list directory
 */
static void ListDirectory(const TCHAR *basePath, const TCHAR *pattern, bool recursive, int maxEntries, json_t *entries, int *count)
{
   _TDIR *dir = _topendir(basePath);
   if (dir == nullptr)
      return;

   struct _tdirent *entry;
   while ((entry = _treaddir(dir)) != nullptr && *count < maxEntries)
   {
      if (!_tcscmp(entry->d_name, _T(".")) || !_tcscmp(entry->d_name, _T("..")))
         continue;

      TCHAR fullPath[MAX_PATH];
      _sntprintf(fullPath, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("%s"), basePath, entry->d_name);

      NX_STAT_STRUCT st;
      if (CALL_STAT_FOLLOW_SYMLINK(fullPath, &st) != 0)
         continue;

      bool isDir = S_ISDIR(st.st_mode);

      // Check if matches pattern
      if (MatchString(pattern, entry->d_name, false))
      {
         json_t *fileEntry = json_object();
         json_object_set_new(fileEntry, "name", json_string_t(entry->d_name));
         json_object_set_new(fileEntry, "path", json_string_t(fullPath));
         json_object_set_new(fileEntry, "size", json_integer(st.st_size));
         json_object_set_new(fileEntry, "mtime", json_integer(st.st_mtime));
         json_object_set_new(fileEntry, "is_directory", json_boolean(isDir));
         json_array_append_new(entries, fileEntry);
         (*count)++;
      }

      // Recurse into directories
      if (isDir && recursive && *count < maxEntries)
      {
         ListDirectory(fullPath, pattern, recursive, maxEntries, entries, count);
      }
   }

   _tclosedir(dir);
}

/**
 * Tool: file_list
 */
uint32_t H_FileList(json_t *params, json_t **result, AbstractCommSession *session)
{
   String directory = json_object_get_string(params, "directory", _T(""));
   if (directory.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'directory' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   String pattern = json_object_get_string(params, "pattern", _T("*"));
   bool recursive = json_object_get_boolean(params, "recursive", false);
   int maxEntries = json_object_get_int32(params, "max_entries", 1000);

   // Check if directory exists
   NX_STAT_STRUCT st;
   if (CALL_STAT(directory, &st) != 0 || !S_ISDIR(st.st_mode))
   {
      SetError(result, "NOT_DIRECTORY", "Path is not a directory or does not exist");
      return ERR_BAD_ARGUMENTS;
   }

   *result = json_object();
   json_t *entries = json_array();
   int count = 0;

   ListDirectory(directory, pattern, recursive, maxEntries, entries, &count);

   json_object_set_new(*result, "entries", entries);
   json_object_set_new(*result, "count", json_integer(count));
   json_object_set_new(*result, "truncated", json_boolean(count >= maxEntries));

   return ERR_SUCCESS;
}

/**
 * Tool: file_read
 */
uint32_t H_FileRead(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int64_t offset = json_object_get_int64(params, "offset", 0);
   int64_t limit = json_object_get_int64(params, "limit", 65536);

   if (limit > 1048576)
      limit = 1048576;  // Cap at 1MB

   FILE *f = _tfopen(file, _T("rb"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Get file size
   fseek(f, 0, SEEK_END);
   int64_t fileSize = ftell(f);

   if (offset >= fileSize)
   {
      fclose(f);
      *result = json_object();
      json_object_set_new(*result, "content", json_string(""));
      json_object_set_new(*result, "bytes_read", json_integer(0));
      json_object_set_new(*result, "offset", json_integer(offset));
      json_object_set_new(*result, "file_size", json_integer(fileSize));
      return ERR_SUCCESS;
   }

   fseek(f, (long)offset, SEEK_SET);

   int64_t toRead = limit;
   if (offset + toRead > fileSize)
      toRead = fileSize - offset;

   char *buffer = MemAllocArray<char>(toRead + 1);
   size_t bytesRead = fread(buffer, 1, (size_t)toRead, f);
   buffer[bytesRead] = 0;
   fclose(f);

   *result = json_object();
   json_object_set_new(*result, "content", json_string(buffer));
   json_object_set_new(*result, "bytes_read", json_integer(bytesRead));
   json_object_set_new(*result, "offset", json_integer(offset));
   json_object_set_new(*result, "file_size", json_integer(fileSize));
   json_object_set_new(*result, "has_more", json_boolean(offset + (int64_t)bytesRead < fileSize));

   MemFree(buffer);
   return ERR_SUCCESS;
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("AIFILEOPS"), NETXMS_VERSION_STRING,
   nullptr, nullptr, nullptr, nullptr, nullptr,
   0, nullptr,    // parameters
   0, nullptr,    // lists
   0, nullptr,    // tables
   0, nullptr,    // actions
   0, nullptr,    // push parameters
   sizeof(s_aiTools) / sizeof(AIToolDefinition),
   s_aiTools
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(AIFILEOPS)
{
   *ppInfo = &s_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
