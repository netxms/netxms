/*
** NetXMS user agent
** Copyright (C) 2009-2024 Raden Solutions
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
** File: config.cpp
**/

#include "nxuseragent.h"
#include <netxms-version.h>

/**
 * "Close on deactivate" flag
 */
bool g_closeOnDeactivate = false;

/**
 * Default application colors
 */
static COLORREF s_defaultColors[12] = {
   RGB(48, 48, 48),        // background
   RGB(192, 192, 192),     // foreground
   RGB(192, 32, 32),       // highlight
   RGB(192, 192, 192),     // border
   RGB(48, 48, 48),        // menu background
   RGB(210, 105, 0),       // menu selected
   RGB(80, 80, 80),        // menu highlighted
   RGB(240, 240, 240),     // menu text
   RGB(240, 240, 240),     // notification background
   RGB(32, 32, 32),        // notification foreground
   RGB(255, 127, 0),       // notification selected
   RGB(192, 192, 192)      // notification highlighted
};

/**
 * Application colors
 */
static COLORREF s_colors[12] = {
   RGB(48, 48, 48),        // background
   RGB(192, 192, 192),     // foreground
   RGB(192, 32, 32),       // highlight
   RGB(192, 192, 192),     // border
   RGB(48, 48, 48),        // menu background
   RGB(210, 105, 0),       // menu selected
   RGB(80, 80, 80),        // menu highlighted
   RGB(240, 240, 240),     // menu text
   RGB(240, 240, 240),     // notification background
   RGB(32, 32, 32),        // notification foreground
   RGB(255, 127, 0),       // notification selected
   RGB(192, 192, 192)      // notification highlighted
};

/**
 * Get application color
 */
COLORREF GetApplicationColor(ApplicationColorId colorId)
{
   return s_colors[colorId];
}

/**
 * Welcome message
 */
static TCHAR *s_welcomeMessage = nullptr;

/**
 * Get welcome message
 */
const TCHAR *GetWelcomeMessage()
{
   return (s_welcomeMessage != nullptr) ? s_welcomeMessage : _T("NetXMS User Agent\r\nVersion ") NETXMS_VERSION_STRING;
}

/**
 * Tooltip message
 */
static TCHAR *s_tooltipMessage = nullptr;

/**
 * Get welcome message
 */
const TCHAR *GetTooltipMessage()
{
   return (s_tooltipMessage != nullptr) ? s_tooltipMessage : _T("NetXMS User Agent");
}

/**
 * Get hotkey
 */
UINT GetHotKey(UINT *modifiers)
{
   *modifiers = MOD_ALT | MOD_SHIFT;
   return VK_MULTIPLY;
}

/**
 * Desktop wallpaper
 */
TCHAR *g_desktopWallpaper = nullptr;

/**
 * Main window position
 */
MainWindowPosition g_mainWindowPosition;

/**
 * Application icon
 */
static HICON s_appIcon = NULL;

/**
 * Acquire application icon
 */
HICON GetApplicationIcon()
{
   return (s_appIcon != NULL) ? s_appIcon : LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));
}

/**
 * Create custom icon from configuration
 */
static void CreateCustomIcon(const TCHAR *data)
{
   char *imageData = NULL;
   size_t imageDataLen = 0;

#ifdef UNICODE
   char *mbdata = MBStringFromWideString(data);
   BOOL success = base64_decode_alloc(mbdata, strlen(mbdata), &imageData, &imageDataLen);
   MemFree(mbdata);
#else
   BOOL success = base64_decode_alloc(data, strlen(data), &imageData, &imageDataLen);
#endif
   if (!success)
      return;

   TCHAR path[MAX_PATH];
   GetTempPath(MAX_PATH, path);

   TCHAR fname[MAX_PATH];
   GetTempFileName(path, _T("nxua"), 0, fname);

   FILE *f = _tfopen(fname, _T("wb"));
   if (f != nullptr)
   {
      fwrite(imageData, 1, imageDataLen, f);
      fclose(f);
   }

   MemFree(imageData);

   if (s_appIcon != nullptr)
      DestroyIcon(s_appIcon);
   s_appIcon = (HICON)LoadImage(nullptr, fname, IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
   if (s_appIcon == nullptr)
      nxlog_debug(2, _T("Cannot load application icon from %s"), fname);

   UpdateTrayIcon(fname);

   DeleteFile(fname);
}

/**
 * Combine text message from multiple policies
 */
static void BuildTextMessageFromConfig(TCHAR **message, const Config& config, const TCHAR *path)
{
   MemFreeAndNull(*message);
   ConfigEntry *e = config.getEntry(path);
   if (e != nullptr)
   {
      StringBuffer buffer;
      for (int i = 0; i < e->getValueCount(); i++)
      {
         const TCHAR *v = e->getValue(i);
         if ((v != nullptr) && (*v != 0))
         {
            if (!buffer.isEmpty())
               buffer.append(_T("\r\n"));
            buffer.append(v);
         }
      }
      if (!buffer.isEmpty())
         *message = MemCopyString(buffer);
   }
}

/**
 * Config merge strategy
 */
static ConfigEntry *SupportAppMergeStrategy(ConfigEntry *parent, const TCHAR *name)
{
   if (!_tcsicmp(name, _T("item")))
      return nullptr;
   return parent->findEntry(name);
}

/**
 * Load configuration. Should be called on UI thread.
 */
void LoadConfig()
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
      _tcscpy(path, _T("C:"));
   _tcscat(path, _T("\\nxuseragent\\config.xml"));

   Config config;
   config.setMergeStrategy(SupportAppMergeStrategy);
   bool success = config.loadXmlConfig(path, "SupportAppPolicy");
   if (success)
   {
      nxlog_debug(2, _T("Configuration loaded from %s"), path);

      BuildTextMessageFromConfig(&s_welcomeMessage, config, _T("/welcomeMessage"));
      BuildTextMessageFromConfig(&s_tooltipMessage, config, _T("/tooltipMessage"));
      UpdateTrayTooltip();

      ConfigEntry *e = config.getEntry(_T("/icon"));
      if (e != nullptr)
      {
         CreateCustomIcon(e->getValue());
      }
      else
      {
         if (s_appIcon != nullptr)
         {
            DestroyIcon(s_appIcon);
            s_appIcon = nullptr;
         }
         ResetTrayIcon();
      }

      g_closeOnDeactivate = config.getValueAsBoolean(_T("/closeOnDeactivate"), false);

      MemFree(g_desktopWallpaper);
      g_desktopWallpaper = MemCopyString(config.getFirstNonEmptyValue(_T("/desktopWallpaper")));

      ConfigEntry *customColorSchema = config.getEntry(_T("/customColorSchema"));
      int customColorSchemaIndex = -1;
      if (customColorSchema != nullptr)
      {
         for (int i = 0; i < customColorSchema->getValueCount(); i++)
            if (customColorSchema->getValueAsBoolean(i))
            {
               customColorSchemaIndex = i;
               break;
            }
      }
      if (customColorSchemaIndex != -1)
      {
         s_colors[APP_COLOR_BACKGROUND] = config.getValueAsUInt(_T("/backgroundColor"), s_defaultColors[APP_COLOR_BACKGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_FOREGROUND] = config.getValueAsUInt(_T("/textColor"), s_defaultColors[APP_COLOR_FOREGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_HIGHLIGHT] = config.getValueAsUInt(_T("/highlightColor"), s_defaultColors[APP_COLOR_HIGHLIGHT], customColorSchemaIndex);
         s_colors[APP_COLOR_BORDER] = config.getValueAsUInt(_T("/borderColor"), s_defaultColors[APP_COLOR_BORDER], customColorSchemaIndex);
         s_colors[APP_COLOR_MENU_BACKGROUND] = config.getValueAsUInt(_T("/menuBackgroundColor"), s_defaultColors[APP_COLOR_MENU_BACKGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_MENU_SELECTED] = config.getValueAsUInt(_T("/menuSelectionColor"), s_defaultColors[APP_COLOR_MENU_SELECTED], customColorSchemaIndex);
         s_colors[APP_COLOR_MENU_HIGHLIGHTED] = config.getValueAsUInt(_T("/menuHighligtColor"), s_defaultColors[APP_COLOR_MENU_HIGHLIGHTED], customColorSchemaIndex);
         s_colors[APP_COLOR_MENU_FOREGROUND] = config.getValueAsUInt(_T("/menuTextColor"), s_defaultColors[APP_COLOR_MENU_FOREGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_NOTIFICATION_BACKGROUND] = config.getValueAsUInt(_T("/notificationBackgroundColor"), s_defaultColors[APP_COLOR_NOTIFICATION_BACKGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_NOTIFICATION_FOREGROUND] = config.getValueAsUInt(_T("/notificationTextColor"), s_defaultColors[APP_COLOR_NOTIFICATION_FOREGROUND], customColorSchemaIndex);
         s_colors[APP_COLOR_NOTIFICATION_SELECTED] = config.getValueAsUInt(_T("/notificationSelectionColor"), s_defaultColors[APP_COLOR_NOTIFICATION_SELECTED], customColorSchemaIndex);
         s_colors[APP_COLOR_NOTIFICATION_HIGHLIGHTED] = config.getValueAsUInt(_T("/notificationHighligtColor"), s_defaultColors[APP_COLOR_NOTIFICATION_HIGHLIGHTED], customColorSchemaIndex);
      }
      else
      {
         memcpy(s_colors, s_defaultColors, sizeof(s_colors));
      }

      int pos = config.getValueAsInt(_T("/mainWindowPosition"), 0);
      if ((pos >= 0x11) && (pos <= 0x33) && ((pos & 15) >= 1) && ((pos & 15) <= 3) && ((pos >> 4) >= 1) && ((pos >> 4) <= 3))
      {
         g_mainWindowPosition = static_cast<MainWindowPosition>(pos);
      }
      else
      {
         g_mainWindowPosition = MainWindowPosition::AUTOMATIC;
      }

      g_notificationTimeout = config.getValueAsUInt(_T("/notificationTimeout"), 60000);

      LoadMenuItems(&config);
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Error loading user agent configuration"));
   }
}

/**
 * Update configuration from master agent
 */
void UpdateConfig(const NXCPMessage *msg)
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
      _tcscpy(path, _T("C:"));
   _tcscat(path, _T("\\nxuseragent"));
   CreateDirectoryTree(path);
   _tcscat(path, _T("\\config.xml"));

   int fd = _topen(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, _S_IREAD | _S_IWRITE);
   if (fd != -1)
   {
      char *data = msg->getFieldAsUtf8String(VID_CONFIG_FILE_DATA);
      if (data != nullptr)
      {
         _write(fd, data, (int)strlen(data));
         MemFree(data);
         g_reloadConfig = true;
      }
      _close(fd);
   }

   // Update path to agent's file store
   if (msg->isFieldExist(VID_FILE_STORE))
   {
      TCHAR fileStore[MAX_PATH];
      msg->getFieldAsString(VID_FILE_STORE, fileStore, MAX_PATH);
      SetEnvironmentVariable(_T("NETXMS_FILE_STORE"), fileStore);

      TCHAR *p = _tcsrchr(path, _T('\\'));
      if (p != nullptr)
      {
         p++;
         _tcscpy(p, _T("filestore"));
         fd = _topen(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, _S_IREAD | _S_IWRITE);
         if (fd != -1)
         {
            _write(fd, fileStore, (int)_tcslen(fileStore) * sizeof(TCHAR));
            _close(fd);
         }
      }
   }
}
