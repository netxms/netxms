/*
** NetXMS - Network Management System
** Command line AI assistant client
** Copyright (C) 2025-2026 Raden Solutions
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
** File: session.cpp
**
**/

#include "nxai.h"
#include <fcntl.h>

/**
 * Name of file with saved sessions
 */
#define SESSION_FILE   _T("sessions.json")

/**
 * Assumed lifetime of saved token in seconds. Server extends token expiration time on every request,
 * so token that was not used for that long will certainly be rejected. Server may invalidate token
 * earlier, in which case saved session is discarded when server responds with "access denied".
 */
#define SESSION_LIFETIME   14400

/**
 * Get full path to file within tool's configuration directory. Configuration directory is created
 * if it does not exist.
 */
bool GetConfigFilePath(const TCHAR *name, TCHAR *path, size_t size)
{
   TCHAR directory[MAX_PATH];
#ifdef _WIN32
   String base = GetEnvironmentVariableEx(_T("APPDATA"));
   if (base.isEmpty())
      return false;
   _sntprintf(directory, MAX_PATH, _T("%s\\nxai"), base.cstr());
#else
   String configHome = GetEnvironmentVariableEx(_T("XDG_CONFIG_HOME"));
   if (!configHome.isEmpty())
   {
      _sntprintf(directory, MAX_PATH, _T("%s/nxai"), configHome.cstr());
   }
   else
   {
      String home = GetEnvironmentVariableEx(_T("HOME"));
      if (home.isEmpty())
         return false;
      _sntprintf(directory, MAX_PATH, _T("%s/.config/nxai"), home.cstr());
   }
#endif

   if (!CreateDirectoryTree(directory))
      return false;

   _sntprintf(path, size, _T("%s") FS_PATH_SEPARATOR _T("%s"), directory, name);
   return true;
}

/**
 * Load saved sessions. Always returns valid JSON object.
 */
static json_t *LoadSessions()
{
   TCHAR fileName[MAX_PATH];
   if (!GetConfigFilePath(SESSION_FILE, fileName, MAX_PATH))
      return json_object();

   char *content = LoadFileAsUTF8String(fileName);
   if (content == nullptr)
      return json_object();

   json_error_t error;
   json_t *sessions = json_loads(content, 0, &error);
   MemFree(content);

   if (!json_is_object(sessions))
   {
      json_decref(sessions);
      return json_object();
   }
   return sessions;
}

/**
 * Save sessions to file readable only by current user
 */
static bool SaveSessions(json_t *sessions)
{
   TCHAR fileName[MAX_PATH];
   if (!GetConfigFilePath(SESSION_FILE, fileName, MAX_PATH))
      return false;

   char *content = json_dumps(sessions, JSON_INDENT(2));
   if (content == nullptr)
      return false;

   bool success = false;
   int fileHandle = _topen(fileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
   if (fileHandle != -1)
   {
      size_t size = strlen(content);
      success = (_write(fileHandle, content, static_cast<unsigned int>(size)) == static_cast<ssize_t>(size));
      _close(fileHandle);
   }
   MemFree(content);
   return success;
}

/**
 * Load saved access token for given server. Expired token is discarded.
 */
bool LoadSessionToken(const char *server, std::string *token)
{
   json_t *sessions = LoadSessions();
   json_t *session = json_object_get(sessions, server);

   bool success = false;
   if (json_is_object(session))
   {
      const char *value = json_object_get_string_utf8(session, "token", nullptr);
      time_t savedAt = static_cast<time_t>(json_object_get_int64(session, "savedAt", 0));
      if ((value != nullptr) && (savedAt + SESSION_LIFETIME > time(nullptr)))
      {
         *token = value;
         success = true;
      }
      else
      {
         json_object_del(sessions, server);
         SaveSessions(sessions);
      }
   }

   json_decref(sessions);
   return success;
}

/**
 * Save access token for given server
 */
bool SaveSessionToken(const char *server, const char *token)
{
   json_t *sessions = LoadSessions();

   json_t *session = json_object();
   json_object_set_new(session, "token", json_string(token));
   json_object_set_new(session, "savedAt", json_integer(time(nullptr)));
   json_object_set_new(sessions, server, session);

   bool success = SaveSessions(sessions);
   json_decref(sessions);
   return success;
}

/**
 * Delete saved session for given server
 */
void ClearSessionToken(const char *server)
{
   json_t *sessions = LoadSessions();
   if (json_object_del(sessions, server) == 0)
      SaveSessions(sessions);
   json_decref(sessions);
}
