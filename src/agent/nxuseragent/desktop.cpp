#include "nxuseragent.h"

/**
 * Draw message on desktop
 */
void DrawTextOnDesktop(const TCHAR *text)
{
   HDC hdc = GetDC(NULL);

   RECT rect;
   rect.left = 10;
   rect.top = 10;
   rect.right = 200;
   rect.bottom = 200;
   DrawText(hdc, text, _tcslen(text), &rect, DT_LEFT | DT_NOPREFIX);

   ReleaseDC(NULL, hdc);
}

/**
 * Update desktop wallpaper
 */
void UpdateDesktopWallpaper()
{
   if (g_desktopWallpaper == NULL)
      return;

   TCHAR wallpaper[MAX_PATH];
   ExpandEnvironmentStrings(g_desktopWallpaper, wallpaper, MAX_PATH);

   nxlog_debug(2, _T("Setting desktop wallpaper to \"%s\""), wallpaper);
   SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, wallpaper, SPIF_SENDCHANGE);
}
