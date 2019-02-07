#include "nxuseragent.h"

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
 * Config merge strategy
 */
static ConfigEntry *SupportAppMergeStrategy(ConfigEntry *parent, const TCHAR *name)
{
   if (!_tcsicmp(name, _T("item")))
      return NULL;
   return parent->findEntry(name);
}

/**
 * Load configuration
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
         String buffer;
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

      LoadMenuItems(&config);
   }
   else
   {
      nxlog_write(MSG_CONFIG_LOAD_FAILED, NXLOG_ERROR, NULL);
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
