#include "nxuseragent.h"

/**
 * "Close on deactivate" flag
 */
bool g_closeOnDeactivate = false;

/**
 * Default application colors
 */
static COLORREF s_defaultColors[8] = {
   RGB(48, 48, 48),        // background
   RGB(192, 192, 192),     // foreground
   RGB(192, 32, 32),       // highlight
   RGB(192, 192, 192),     // border
   RGB(48, 48, 48),        // menu background
   RGB(210, 105, 0),       // menu selected
   RGB(80, 80, 80),        // menu highlighted
   RGB(240, 240, 240)      // menu text
};

/**
 * Application colors
 */
static COLORREF s_colors[8] = {
   RGB(48, 48, 48),        // background
   RGB(192, 192, 192),     // foreground
   RGB(192, 32, 32),       // highlight
   RGB(192, 192, 192),     // border
   RGB(48, 48, 48),        // menu background
   RGB(210, 105, 0),       // menu selected
   RGB(80, 80, 80),        // menu highlighted
   RGB(240, 240, 240)      // menu text
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
static TCHAR *s_welcomeMessage = NULL;

/**
 * Get welcome message
 */
const TCHAR *GetWelcomeMessage()
{
   return (s_welcomeMessage != NULL) ? s_welcomeMessage : _T("NetXMS User Agent\r\nVersion ") NETXMS_VERSION_STRING;
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
   if (f != NULL)
   {
      fwrite(imageData, 1, imageDataLen, f);
      fclose(f);
   }

   MemFree(imageData);

   if (s_appIcon != NULL)
      DestroyIcon(s_appIcon);
   s_appIcon = (HICON)LoadImage(NULL, fname, IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
   if (s_appIcon == NULL)
      nxlog_debug(2, _T("Cannot load application icon from %s"), fname);

   UpdateTrayIcon(fname);

   DeleteFile(fname);
}

/**
 * Config merge strategy
 */
static ConfigEntry *SupportAppMergeStrategy(ConfigEntry *parent, const TCHAR *name)
{
   if (!_tcsicmp(name, _T("item")))
      return NULL;
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
   
      MemFreeAndNull(s_welcomeMessage);
      ConfigEntry *e = config.getEntry(_T("/welcomeMessage"));
      if (e != NULL)
      {
         StringBuffer buffer;
         for (int i = 0; i < e->getValueCount(); i++)
         {
            const TCHAR *v = e->getValue(i);
            if ((v != NULL) && (*v != 0))
            {
               if (!buffer.isEmpty())
                  buffer.append(_T("\r\n"));
               buffer.append(v);
            }
         }
         if (!buffer.isEmpty())
            s_welcomeMessage = MemCopyString(buffer);
      }

      e = config.getEntry(_T("/icon"));
      if (e != NULL)
      {
         CreateCustomIcon(e->getValue());
      }
      else
      {
         if (s_appIcon != NULL)
         {
            DestroyIcon(s_appIcon);
            s_appIcon = NULL;
         }
         ResetTrayIcon();
      }

      g_closeOnDeactivate = config.getValueAsBoolean(_T("/closeOnDeactivate"), false);

      if (config.getValueAsBoolean(_T("/customColorSchema"), false))
      {
         s_colors[APP_COLOR_BACKGROUND] = config.getValueAsUInt(_T("/backgroundColor"), s_defaultColors[APP_COLOR_BACKGROUND]);
         s_colors[APP_COLOR_FOREGROUND] = config.getValueAsUInt(_T("/textColor"), s_defaultColors[APP_COLOR_FOREGROUND]);
         s_colors[APP_COLOR_HIGHLIGHT] = config.getValueAsUInt(_T("/highlightColor"), s_defaultColors[APP_COLOR_HIGHLIGHT]);
         s_colors[APP_COLOR_BORDER] = config.getValueAsUInt(_T("/borderColor"), s_defaultColors[APP_COLOR_BORDER]);
         s_colors[APP_COLOR_MENU_BACKGROUND] = config.getValueAsUInt(_T("/menuBackgroundColor"), s_defaultColors[APP_COLOR_MENU_BACKGROUND]);
         s_colors[APP_COLOR_MENU_SELECTED] = config.getValueAsUInt(_T("/menuSelectionColor"), s_defaultColors[APP_COLOR_MENU_SELECTED]);
         s_colors[APP_COLOR_MENU_HIGHLIGHTED] = config.getValueAsUInt(_T("/menuHighligtColor"), s_defaultColors[APP_COLOR_MENU_HIGHLIGHTED]);
         s_colors[APP_COLOR_MENU_FOREGROUND] = config.getValueAsUInt(_T("/menuTextColor"), s_defaultColors[APP_COLOR_MENU_FOREGROUND]);
      }
      else
      {
         memcpy(s_colors, s_defaultColors, sizeof(s_colors));
      }

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
   CreateFolder(path);
   _tcscat(path, _T("\\config.xml"));

   int fd = _topen(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, _S_IREAD | _S_IWRITE);
   if (fd != -1)
   {
      char *data = msg->getFieldAsUtf8String(VID_CONFIG_FILE_DATA);
      if (data != NULL)
      {
         _write(fd, data, (int)strlen(data));
         MemFree(data);
         g_reloadConfig = true;
      }
      _close(fd);
   }
}
