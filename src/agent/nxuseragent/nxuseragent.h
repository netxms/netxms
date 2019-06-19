#ifndef _nxuseragent_h_
#define _nxuseragent_h_

#define _WIN32_WINNT                0x0600
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
#include "messages.h"

/*** Messages ***/
#define NXUA_MSG_TOOLTIP_NOTIFY        (WM_USER + 1)
#define NXUA_MSG_OPEN_MESSAGE_WINDOW   (WM_USER + 2)

/*** Main window parameters ***/
#define MARGIN_WIDTH             15
#define BORDER_WIDTH             1
#define WINDOW_VERTICAL_SPACING  12
#define MENU_VERTICAL_SPACING    12
#define INFO_AREA_SPACING        20
#define MENU_HIGHLIGHT_MARGIN    2
#define HEADER_HEIGHT            16
#define FOOTER_HEIGHT            16
#define INDICATOR_HEIGHT         8
#define APP_ICON_SIZE            64
#define APP_ICON_SPACING         12

#define FONT_SIZE_MESSAGE        12
#define FONT_SIZE_MENU           13
#define FONT_SIZE_SYMBOLS        12
#define FONT_SIZE_FOOTER         8

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
   APP_COLOR_MENU_FOREGROUND = 7
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
   DISABLE_COPY_CTOR(MenuItem);

private:
   TCHAR *m_name;
   TCHAR *m_displayName;
   TCHAR *m_path;
   TCHAR *m_command;
   MenuItem *m_parent;
   ObjectArray<MenuItem> *m_subItems;
   HWND m_hWnd;
   bool m_highlighted;
   bool m_selected;

   void loadSubItems(ConfigEntry *config);
   void draw(HDC hdc) const;
   void trackMouseEvent();

public:
   MenuItem();
   MenuItem(MenuItem *parent);
   MenuItem(MenuItem *parent, ConfigEntry *config, const TCHAR *rootPath);
   ~MenuItem();

   void loadRootMenu(ConfigEntry *config) { if (*m_path == 0) { m_subItems->clear(); loadSubItems(config); } }
   void setWindowHandle(HWND hWnd) { m_hWnd = hWnd; }

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return m_displayName; }
   const TCHAR *getPath() const { return m_path; }
   bool isTopLevelMenu() const { return m_parent == NULL; }
   bool isSubMenu() const { return m_subItems != NULL; }
   bool isEmptySubMenu() const { return isSubMenu() && (m_subItems->size() <= 1);  }

   int getItemCount() const { return (m_subItems != NULL) ? m_subItems->size() : 0; }
   MenuItem *getItemAtPos(int pos) { return (m_subItems != NULL) ? m_subItems->get(pos) : NULL; }
   int getItemPos(MenuItem *item) { return (m_subItems != NULL) ? m_subItems->indexOf(item) : 0; }
   
   POINT calculateSize(HDC hdc) const;

   void activate(HWND hParentWnd);
   void deactivate();
   void execute();
   void setSelected(bool selected);

   LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};

/*** Functions ***/
bool SetupTrayIcon();
void RemoveTrayIcon();
void ResetTrayIcon();
void UpdateTrayIcon(const TCHAR *file);
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
void CloseApplicationWindow();
COLORREF GetApplicationColor(ApplicationColorId colorId);
const TCHAR *GetWelcomeMessage();
HICON GetApplicationIcon();
POINT GetMenuPosition();
POINT GetTextExtent(HDC hdc, const TCHAR *text, HFONT font = NULL);
void UpdateAddressList();
int GetAddressListSize();
StructArray<InetAddress> *GetAddressList();
UINT GetHotKey(UINT *modifiers);
void TakeScreenshot(NXCPMessage *response);
void StartAgentConnector();
void StopAgentConnector();
void SendLoginMessage();
void UpdateNotifications(NXCPMessage *request);
void AddNotification(NXCPMessage *request);
void RemoveNotification(NXCPMessage *request);
ObjectArray<UserAgentNotification> *GetNotificationsForDisplay();
void ShowTrayNotification(const TCHAR *text);
bool PrepareMessageWindow();
void OpenMessageWindow();

/*** Global variables ***/
extern HINSTANCE g_hInstance;
extern DWORD g_mainThreadId;
extern HFONT g_menuFont;
extern HFONT g_messageFont;
extern HFONT g_symbolFont;
extern bool g_reloadConfig;
extern bool g_connectedToAgent;

#endif
