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
** File: nxuseragent.h
**/

#ifndef _nxuseragent_h_
#define _nxuseragent_h_

#define _WIN32_WINNT                0x0601
#define SECURITY_WIN32

#include <nms_common.h>
#include <nms_util.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include <security.h>
#include <secext.h>
#include <nxconfig.h>
#include <nms_agent.h>
#include <windowsx.h>
#include "resource.h"

/*** Messages ***/
#define NXUA_MSG_TOOLTIP_NOTIFY        (WM_USER + 1)
#define NXUA_MSG_SHOW_NOTIFICATIONS    (WM_USER + 2)

/*** Main window parameters ***/
#define MARGIN_WIDTH               15
#define MARGIN_HEIGHT              15
#define BORDER_WIDTH               1
#define WINDOW_VERTICAL_SPACING    12
#define WINDOW_HORIZONTAL_SPACING  12
#define MENU_VERTICAL_SPACING      12
#define INFO_AREA_SPACING          20
#define MENU_HIGHLIGHT_MARGIN      2
#define HEADER_HEIGHT              16
#define FOOTER_HEIGHT              16
#define INDICATOR_HEIGHT           8
#define APP_ICON_SIZE              64
#define APP_ICON_SPACING           12
#define BUTTON_MARGIN_WIDTH        3
#define BUTTON_MARGIN_HEIGHT       3

#define FONT_SIZE_MESSAGE          12
#define FONT_SIZE_MENU             13
#define FONT_SIZE_SYMBOLS          12
#define FONT_SIZE_FOOTER           8
#define FONT_SIZE_NOTIFICATION     13

#define MENU_ITEM_CLASS_NAME  _T("NetXMS_UA_MenuItem")

/**
 * Main window positions
 */
enum class MainWindowPosition
{
   AUTOMATIC = 0,
   TOP_LEFT = 0x11,
   TOP_CENTER = 0x12,
   TOP_RIGHT = 0x13,
   MIDDLE_LEFT = 0x21,
   MIDDLE_CENTER = 0x22,
   MIDDLE_RIGHT = 0x23,
   BOTTOM_LEFT = 0x31,
   BOTTOM_CENTER = 0x32,
   BOTTOM_RIGHT = 0x33
};

/**
 * Color identifiers
 */
enum ApplicationColorId
{
   APP_COLOR_BACKGROUND = 0,
   APP_COLOR_FOREGROUND = 1,
   APP_COLOR_HIGHLIGHT = 2,
   APP_COLOR_BORDER = 3,
   APP_COLOR_MENU_BACKGROUND = 4,
   APP_COLOR_MENU_SELECTED = 5,
   APP_COLOR_MENU_HIGHLIGHTED = 6,
   APP_COLOR_MENU_FOREGROUND = 7,
   APP_COLOR_NOTIFICATION_BACKGROUND = 8,
   APP_COLOR_NOTIFICATION_FOREGROUND = 9,
   APP_COLOR_NOTIFICATION_SELECTED = 10,
   APP_COLOR_NOTIFICATION_HIGHLIGHTED = 11
};

/**
 * User session information
 */
struct UserSession
{
   DWORD sid;
   int state;
   TCHAR name[256];
   TCHAR user[256];
   TCHAR computer[256];
   TCHAR client[256];
};

/**
 * User menu item
 */
class MenuItem
{
protected:
   TCHAR *m_name;
   TCHAR *m_displayName;
   TCHAR *m_path;
   TCHAR *m_command;
   MenuItem *m_parent;
   ObjectArray<MenuItem> *m_subItems;
   HWND m_hWnd;
   bool m_highlighted;
   bool m_selected;
   bool m_standalone;
   bool m_symbol;

   void loadSubItems(ConfigEntry *config);
   void merge(MenuItem *sourceItem);
   void draw(HDC hdc) const;
   void trackMouseEvent();

public:
   MenuItem();
   MenuItem(const TCHAR *text, bool symbol);
   MenuItem(MenuItem *parent);
   MenuItem(MenuItem *parent, ConfigEntry *config, const TCHAR *rootPath);
   MenuItem(const MenuItem& src) = delete;
   virtual ~MenuItem();

   void loadRootMenu(ConfigEntry *config) { if (*m_path == 0) { m_subItems->clear(); loadSubItems(config); } }
   void setWindowHandle(HWND hWnd) { m_hWnd = hWnd; }

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return m_displayName; }
   const TCHAR *getPath() const { return m_path; }
   bool isTopLevelMenu() const { return m_parent == NULL; }
   bool isSubMenu() const { return m_subItems != NULL; }
   bool isEmptySubMenu() const { return isSubMenu() && (m_subItems->size() <= 1);  }
   bool isBackMenu() const { return (m_command != nullptr) && (_tcscmp(m_command, _T("\x21A9")) == 0); }

   int getItemCount() const { return (m_subItems != NULL) ? m_subItems->size() : 0; }
   MenuItem *getItemAtPos(int pos) { return (m_subItems != NULL) ? m_subItems->get(pos) : NULL; }
   int getItemPos(MenuItem *item) { return (m_subItems != NULL) ? m_subItems->indexOf(item) : 0; }
   
   POINT calculateSize(HDC hdc) const;
   void setSelected(bool selected);

   virtual void activate(HWND hParentWnd);
   virtual void deactivate();
   virtual void execute();
   
   virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};

/**
 * User menu item
 */
class Button
{
protected:
   TCHAR *m_text;
   HWND m_hWnd;
   bool m_highlighted;
   bool m_selected;
   bool m_symbol;
   void (*m_handler)(void*);
   void *m_context;
   COLORREF m_backgroundColor;
   COLORREF m_foregroundColor;
   COLORREF m_selectionColor;
   COLORREF m_highlightColor;

   void draw(HDC hdc) const;
   void trackMouseEvent();

public:
   Button(HWND parent, RECT pos, const TCHAR *text, bool symbol, void (*handler)(void*) = nullptr, void *context = nullptr);
   Button(const Button& src) = delete;
   virtual ~Button();

   void setSelected(bool selected);
   void setBackgroundColor(COLORREF color) { m_backgroundColor = color; }
   void setForegroundColor(COLORREF color) { m_foregroundColor = color; }
   void setSelectionColor(COLORREF color) { m_selectionColor = color; }
   void setHighlightColor(COLORREF color) { m_highlightColor = color; }

   virtual void execute();
   virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};

/*** Functions ***/
bool SetupTrayIcon();
void RemoveTrayIcon();
void ResetTrayIcon();
void UpdateTrayIcon(const TCHAR *file);
void UpdateTrayTooltip();
HWND GetTrayWindow();
bool PrepareApplicationWindow();
bool SetupSessionEventHandler();
void GetSessionInformation(UserSession *buffer);
void LoadConfig();
void UpdateConfig(const NXCPMessage *msg);
bool InitMenu();
void LoadMenuItems(Config *config);
POINT CalculateMenuSize(HWND hWnd);
void MoveMenuCursor(bool backward);
void ResetMenuCursor();
void ActivateMenuItem(HWND hParentWnd, MenuItem *item, MenuItem *selection = NULL);
void ExecuteCurrentMenuItem();
void ExecuteReturnToParentMenu();
void OpenApplicationWindow(POINT pt, bool hotkeyOpen);
void CloseApplicationWindow(void *context = nullptr);
COLORREF GetApplicationColor(ApplicationColorId colorId);
const TCHAR *GetWelcomeMessage();
const TCHAR *GetTooltipMessage();
HICON GetApplicationIcon();
POINT GetMenuPosition();
POINT GetTextExtent(HDC hdc, const TCHAR *text, HFONT font = NULL);
void UpdateAddressList();
int GetAddressListSize();
unique_ptr<StructArray<InetAddress>> GetAddressList();
UINT GetHotKey(UINT *modifiers);
void TakeScreenshot(NXCPMessage *response);
void StartAgentConnector();
void StopAgentConnector();
void SendLoginMessage();
THREAD_RESULT THREAD_CALL NotificationManager(void *arg);
void UpdateNotifications(const NXCPMessage *request);
void AddNotification(const NXCPMessage *request);
void RemoveNotification(const NXCPMessage *request);
ObjectArray<UserAgentNotification> *GetNotificationsForDisplay(bool startup);
void MarkNotificationAsRead(const ServerObjectKey& id);
void ShowPendingNotifications(bool startup);
void ShowTrayNotification(const TCHAR *text);
bool PrepareNotificationWindows();
bool InitButtons();
void DrawTextOnDesktop(const TCHAR *text);
void UpdateDesktopWallpaper();

/*** Global variables ***/
extern HINSTANCE g_hInstance;
extern DWORD g_mainThreadId;
extern Condition g_shutdownCondition;
extern HFONT g_menuFont;
extern HFONT g_messageFont;
extern HFONT g_notificationFont;
extern HFONT g_symbolFont;
extern bool g_reloadConfig;
extern bool g_connectedToAgent;
extern bool g_closeOnDeactivate;
extern TCHAR *g_desktopWallpaper;
extern MainWindowPosition g_mainWindowPosition;
extern uint32_t g_notificationTimeout;

#endif
