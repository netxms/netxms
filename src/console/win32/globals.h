/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: globals.h
**
**/

#ifndef _globals_h_
#define _globals_h_


//
// Application object declaration
//

class CConsoleApp;
extern CConsoleApp theApp;


//
// Constants
//

#define MAX_SERVER_NAME_LEN   64
#define MAX_LOGIN_NAME_LEN    64
#define MAX_PASSWORD_LEN      64
#define MAX_INTERFACE_TYPE    32


//
// User interface options
//

#define UI_OPT_EXPAND_CTRLPANEL  0x0001
#define UI_OPT_SHOW_GRID         0x0002


//
// Transparent color for images in rule list
//

#define PSYM_MASK_COLOR       RGB(255, 255, 255)


//
// Custom windows messages
//

#define WM_REQUEST_COMPLETED     (WM_USER + 1)
#define WM_CLOSE_STATUS_DLG      (WM_USER + 2)
#define WM_OBJECT_CHANGE         (WM_USER + 3)
#define WM_FIND_OBJECT           (WM_USER + 4)
#define WM_EDITBOX_EVENT         (WM_USER + 5)
#define WM_SET_INFO_TEXT         (WM_USER + 6)
#define WM_USERDB_CHANGE         (WM_USER + 7)
#define WM_STATE_CHANGE          (WM_USER + 8)
#define WM_ALARM_UPDATE          (WM_USER + 9)
#define WM_POLLER_MESSAGE        (WM_USER + 10)


//
// Timestamp formats
//

#define TS_LONG_DATE_TIME  0
#define TS_LONG_TIME       1


//
// Subdirectories in working directory
//

#define WORKDIR_MIBCACHE   "\\MIBCache"
#define WORKDIR_IMAGECACHE "\\ImageCache"


//
// Request parameters structure
//

struct RqData
{
   HWND hWnd;
   DWORD (* pFunc)(...);
   DWORD dwNumParams;
   void *pArg1;
   void *pArg2;
   void *pArg3;
   void *pArg4;
   void *pArg5;
   void *pArg6;
   void *pArg7;
};


//
// Code translation structure
//

struct CODE_TO_TEXT
{
   int iCode;
   char *pszText;
};


//
// Default image table
//

struct DEF_IMG
{
   DWORD dwObjectClass;
   DWORD dwImageId;
   int iImageIndex;
};


//
// Communication functions
//

DWORD DoLogin(void);
DWORD DoRequest(DWORD (* pFunc)(void), char *pszInfoText);
DWORD DoRequestArg1(void *pFunc, void *pArg1, char *pszInfoText);
DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, char *pszInfoText);
DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, char *pszInfoText);
DWORD DoRequestArg4(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, char *pszInfoText);
DWORD DoRequestArg5(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, char *pszInfoText);
DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, char *pszInfoText);
DWORD DoRequestArg7(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, char *pszInfoText);
DWORD WINAPI PollerThread(void *pArg);


//
// Drawing functions
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors);
void Draw3dRect(HDC hDC, LPRECT pRect, COLORREF rgbTop, COLORREF rgbBottom);


//
// Image and image list functions
//

void CreateObjectImageList(void);
int ImageIdToIndex(DWORD dwImageId);
int GetObjectImageIndex(NXC_OBJECT *pObject);
CImageList *CreateEventImageList(void);
void LoadBitmapIntoList(CImageList *pImageList, UINT nIDResource, COLORREF rgbMaskColor);


//
// Utility functions
//

char *FormatTimeStamp(DWORD dwTimeStamp, char *pszBuffer, int iType);
CSize GetWindowSize(CWnd *pWnd);
void SelectListViewItem(CListCtrl *pListCtrl, int iItem);
const char *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const char *pszDefaultText = "Unknown");
char *TranslateUNIXText(const char *pszText);
void RestoreMDIChildPlacement(CMDIChildWnd *pWnd, WINDOWPLACEMENT *pwp);
void EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable);


//
// MIB functions
//

BOOL CreateMIBTree(void);
void DestroyMIBTree(void);


//
// Action functions
//

NXC_ACTION *FindActionById(DWORD dwActionId);
void UpdateActions(DWORD dwCode, NXC_ACTION *pAction);


//
// Variables
//

extern NXC_SESSION g_hSession;
extern DWORD g_dwOptions;
extern char g_szServer[];
extern char g_szLogin[];
extern char g_szPassword[];
extern DWORD g_dwEncryptionMethod;
extern char g_szWorkDir[];
extern TCHAR *g_szStatusText[];
extern TCHAR *g_szStatusTextSmall[];
extern TCHAR *g_szActionType[];
extern TCHAR *g_szServiceType[];
extern COLORREF g_statusColorTable[];
extern char *g_szObjectClass[];
extern char *g_szInterfaceTypes[];
extern char *g_pszItemOrigin[];
extern char *g_pszItemOriginLong[];
extern char *g_pszItemDataType[];
extern char *g_pszItemStatus[];
extern char *g_pszThresholdOperation[];
extern char *g_pszThresholdOperationLong[];
extern char *g_pszThresholdFunction[];
extern char *g_pszThresholdFunctionLong[];
extern CODE_TO_TEXT g_ctSnmpMibStatus[];
extern CODE_TO_TEXT g_ctSnmpMibAccess[];
extern CODE_TO_TEXT g_ctSnmpMibType[];
extern NXC_IMAGE_LIST *g_pSrvImageList;
extern CImageList *g_pObjectSmallImageList;
extern CImageList *g_pObjectNormalImageList;
extern DWORD g_dwDefImgListSize;
extern DEF_IMG *g_pDefImgList;
extern DWORD g_dwNumActions;
extern NXC_ACTION *g_pActionList;
extern HANDLE g_mutexActionListAccess;
extern NXC_CC_LIST *g_pCCList;
extern CODE_TO_TEXT g_ctNodeType[];


//
// Lock/unlock access to action list
//

inline void LockActions(void)
{
   WaitForSingleObject(g_mutexActionListAccess, INFINITE);
}

inline void UnlockActions(void)
{
   ReleaseMutex(g_mutexActionListAccess);
}

#endif
