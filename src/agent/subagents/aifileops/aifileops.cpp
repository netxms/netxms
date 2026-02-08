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

static AIToolParameter s_fileWriteParams[] =
{
   { "file", "string", "Path to file to write", true, nullptr, nullptr },
   { "content", "string", "Content to write to the file", true, nullptr, nullptr },
   { "create_backup", "boolean", "Create .bak backup before overwriting existing file", false, "true", nullptr },
   { "create_dirs", "boolean", "Create parent directories if they don't exist", false, "false", nullptr }
};

static AIToolParameter s_fileAppendParams[] =
{
   { "file", "string", "Path to file to append to", true, nullptr, nullptr },
   { "content", "string", "Content to append", true, nullptr, nullptr },
   { "create_if_missing", "boolean", "Create file if it doesn't exist", false, "false", nullptr },
   { "add_newline", "boolean", "Add newline before content if file doesn't end with one", false, "true", nullptr }
};

static AIToolParameter s_fileInsertParams[] =
{
   { "file", "string", "Path to file", true, nullptr, nullptr },
   { "line", "integer", "Line number to insert at (1-based, content inserted before this line)", true, nullptr, "{\"minimum\":1}" },
   { "content", "string", "Content to insert (can contain multiple lines)", true, nullptr, nullptr },
   { "create_backup", "boolean", "Create .bak backup before modifying", false, "true", nullptr }
};

static AIToolParameter s_fileDeleteLinesParams[] =
{
   { "file", "string", "Path to file", true, nullptr, nullptr },
   { "start_line", "integer", "First line to delete (1-based)", true, nullptr, "{\"minimum\":1}" },
   { "end_line", "integer", "Last line to delete (inclusive)", true, nullptr, "{\"minimum\":1}" },
   { "create_backup", "boolean", "Create .bak backup before modifying", false, "true", nullptr }
};

static AIToolParameter s_fileReplaceParams[] =
{
   { "file", "string", "Path to file", true, nullptr, nullptr },
   { "pattern", "string", "Regular expression pattern to find", true, nullptr, nullptr },
   { "replacement", "string", "Replacement text (supports $1, $2, etc. for backreferences)", true, nullptr, nullptr },
   { "max_replacements", "integer", "Maximum number of replacements (0 = unlimited)", false, "0", "{\"minimum\":0}" },
   { "case_insensitive", "boolean", "Perform case-insensitive matching", false, "false", nullptr },
   { "create_backup", "boolean", "Create .bak backup before modifying", false, "true", nullptr },
   { "dry_run", "boolean", "Preview changes without modifying the file", false, "false", nullptr }
};

static AIToolParameter s_filePatchParams[] =
{
   { "file", "string", "Path to file to patch", true, nullptr, nullptr },
   { "diff", "string", "Unified diff content to apply", true, nullptr, nullptr },
   { "create_backup", "boolean", "Create .bak backup before modifying", false, "true", nullptr },
   { "fuzz", "integer", "Number of context lines that can differ when matching", false, "2", "{\"minimum\":0,\"maximum\":10}" }
};

/**
 * Tool definitions
 */
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
   },
   {
      "file_write",
      "file",
      "Write content to a file, creating it if it doesn't exist or overwriting if it does. Creates a backup by default before overwriting. Use this for creating new files or completely replacing file contents.",
      s_fileWriteParams,
      sizeof(s_fileWriteParams) / sizeof(AIToolParameter),
      H_FileWrite
   },
   {
      "file_append",
      "file",
      "Append content to the end of an existing file. Optionally adds a newline if the file doesn't end with one. Use this for adding entries to log files or appending configuration.",
      s_fileAppendParams,
      sizeof(s_fileAppendParams) / sizeof(AIToolParameter),
      H_FileAppend
   },
   {
      "file_insert",
      "file",
      "Insert content at a specific line number in a file. The content is inserted before the specified line. Use this to add lines in the middle of a file.",
      s_fileInsertParams,
      sizeof(s_fileInsertParams) / sizeof(AIToolParameter),
      H_FileInsert
   },
   {
      "file_delete_lines",
      "file",
      "Delete a range of lines from a file. Specify start and end line numbers (1-based, inclusive). Use this to remove specific lines from configuration files or logs.",
      s_fileDeleteLinesParams,
      sizeof(s_fileDeleteLinesParams) / sizeof(AIToolParameter),
      H_FileDeleteLines
   },
   {
      "file_replace",
      "file",
      "Find and replace text in a file using regular expressions. Supports backreferences ($1, $2, etc.) in replacement text. Use dry_run to preview changes without modifying the file.",
      s_fileReplaceParams,
      sizeof(s_fileReplaceParams) / sizeof(AIToolParameter),
      H_FileReplace
   },
   {
      "file_patch",
      "file",
      "Apply a unified diff patch to a file. Supports fuzz factor for flexible context matching. Use this to apply patches or make complex multi-line changes.",
      s_filePatchParams,
      sizeof(s_filePatchParams) / sizeof(AIToolParameter),
      H_FilePatch
   }
};

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
