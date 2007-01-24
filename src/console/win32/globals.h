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
#define MAX_INTERFACE_TYPE    56
#define MAX_WND_PARAM_LEN     32768

#define TOOLBOX_X_MARGIN      5
#define TOOLBOX_Y_MARGIN      5

#define OBJTOOL_MENU_FIRST_ID 4000
#define OBJTOOL_MENU_LAST_ID  4999

#define NXCON_ALARM_SOUND_KEY _T("Software\\NetXMS\\NetXMS Console\\AlarmSounds")

#define NXCON_CONFIG_VERSION  1


//
// Window classes
//

#define WNDC_LAST_VALUES      1
#define WNDC_GRAPH            2
#define WNDC_DCI_DATA         3
#define WNDC_NETWORK_SUMMARY  4
#define WNDC_ALARM_BROWSER    5
#define WNDC_EVENT_BROWSER    6
#define WNDC_OBJECT_BROWSER   7
#define WNDC_SYSLOG_BROWSER   8
#define WNDC_TRAP_LOG_BROWSER 9
#define WNDC_CONTROL_PANEL    10


//
// User interface options
//

#define UI_OPT_EXPAND_CTRLPANEL     0x0001
#define UI_OPT_SHOW_GRID            0x0002
#define UI_OPT_CONFIRM_OBJ_DELETE   0x0004


//
// Other options
//

#define OPT_MATCH_SERVER_VERSION 0x00010000
#define OPT_DONT_CACHE_OBJECTS   0x00020000
#define OPT_ENCRYPT_CONNECTION   0x00040000
#define OPT_CLEAR_OBJECT_CACHE   0x00080000


//
// Transparent color for images in rule list
//

#define PSYM_MASK_COLOR       RGB(255, 255, 255)


//
// Custom windows messages
//

#define NXCM_REQUEST_COMPLETED     (WM_USER + 101)
#define NXCM_CLOSE_STATUS_DLG      (WM_USER + 102)
#define NXCM_OBJECT_CHANGE         (WM_USER + 103)
#define NXCM_FIND_OBJECT           (WM_USER + 104)
#define NXCM_EDITBOX_EVENT         (WM_USER + 105)
#define NXCM_SET_INFO_TEXT         (WM_USER + 106)
#define NXCM_USERDB_CHANGE         (WM_USER + 107)
#define NXCM_STATE_CHANGE          (WM_USER + 108)
#define NXCM_ALARM_UPDATE          (WM_USER + 109)
#define NXCM_POLLER_MESSAGE        (WM_USER + 110)
#define NXCM_START_DEPLOYMENT      (WM_USER + 111)
#define NXCM_DEPLOYMENT_INFO       (WM_USER + 112)
#define NXCM_DEPLOYMENT_FINISHED   (WM_USER + 113)
#define NXCM_GET_SAVE_INFO         (WM_USER + 114)
#define NXCM_TABLE_DATA            (WM_USER + 115)
#define NXCM_NETXMS_EVENT          (WM_USER + 116)
#define NXCM_SYSLOG_RECORD         (WM_USER + 117)
#define NXCM_UPDATE_EVENT_LIST     (WM_USER + 118)
#define NXCM_UPDATE_OBJECT_TOOLS   (WM_USER + 119)
#define NXCM_UPDATE_GRAPH_POINT    (WM_USER + 120)
#define NXCM_TRAP_LOG_RECORD       (WM_USER + 121)
#define NXCM_SUBMAP_CHANGE         (WM_USER + 122)
#define NXCM_CHANGE_PASSWORD       (WM_USER + 123)
#define NXCM_GRAPH_ZOOM_CHANGED    (WM_USER + 124)
#define NXCM_SET_OBJECT            (WM_USER + 125)


//
// Timestamp formats
//

#define TS_LONG_DATE_TIME  0
#define TS_LONG_TIME       1
#define TS_DAY_AND_MONTH   2
#define TS_MONTH           3


//
// Subdirectories and files in working directory
//

#define WORKDIR_MIBCACHE      _T("\\MIBCache")
#define WORKDIR_IMAGECACHE    _T("\\ImageCache")
#define WORKDIR_BKIMAGECACHE  _T("\\BkImageCache")
#define WORKFILE_OBJECTCACHE  _T("\\objects.cache.")


//
// libnxcl object index structure
//

struct NXC_OBJECT_INDEX
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
};


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
   void *pArg8;
   void *pArg9;
};


//
// Code translation structure
//

struct CODE_TO_TEXT
{
   int iCode;
   TCHAR *pszText;
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
// Package deployment startup info
//

struct DEPLOYMENT_JOB
{
   DWORD dwPkgId;
   DWORD dwNumObjects;
   DWORD *pdwObjectList;
   HWND hWnd;
   DWORD *pdwRqId;
};


//
// Window state saving information
//

struct WINDOW_SAVE_INFO
{
   int iWndClass;
   WINDOWPLACEMENT placement;
   TCHAR szParameters[MAX_WND_PARAM_LEN];
};


//
// DCI information
//

class DCIInfo
{
   // Attributes
public:
   DWORD m_dwNodeId;
   DWORD m_dwItemId;
   int m_iSource;
   int m_iDataType;
   TCHAR *m_pszParameter;
   TCHAR *m_pszDescription;

   // Methods
public:
   DCIInfo();
   DCIInfo(DCIInfo *pSrc);
   DCIInfo(DWORD dwNodeId, NXC_DCI *pItem);
   ~DCIInfo();
};


//
// Communication functions
//

DWORD DoLogin(BOOL bClearCache);
DWORD DoRequest(DWORD (* pFunc)(void), TCHAR *pszInfoText);
DWORD DoRequestArg1(void *pFunc, void *pArg1, TCHAR *pszInfoText);
DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, TCHAR *pszInfoText);
DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, TCHAR *pszInfoText);
DWORD DoRequestArg4(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4,
                    TCHAR *pszInfoText);
DWORD DoRequestArg5(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, TCHAR *pszInfoText);
DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, TCHAR *pszInfoText);
DWORD DoRequestArg7(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, TCHAR *pszInfoText);
DWORD DoRequestArg9(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, void *pArg8, void *pArg9,
                    TCHAR *pszInfoText);
DWORD WINAPI PollerThread(void *pArg);
BOOL DownloadUpgradeFile(HANDLE hFile);


//
// Drawing functions
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors);
void Draw3dRect(HDC hDC, LPRECT pRect, COLORREF rgbTop, COLORREF rgbBottom);
void DrawHeading(CDC &dc, TCHAR *pszText, CFont *pFont, RECT *pRect,
                 COLORREF rgbColor1, COLORREF rgbColor2);


//
// Printing functions
//

void PrintBitmap(CDC &dc, CBitmap &bitmap, RECT *pRect);


//
// Image and image list functions
//

void CreateObjectImageList(void);
int ImageIdToIndex(DWORD dwImageId);
int GetObjectImageIndex(NXC_OBJECT *pObject);
int GetClassDefaultImageIndex(int iClass);
CImageList *CreateEventImageList(void);
void LoadBitmapIntoList(CImageList *pImageList, UINT nIDResource, COLORREF rgbMaskColor);
HBITMAP LoadPicture(TCHAR *pszFile, int nScaleFactor);


//
// MIB functions
//

void BuildOIDString(SNMP_MIBObject *pNode, TCHAR *pszBuffer);
TCHAR *BuildSymbolicOIDString(SNMP_MIBObject *pNode, DWORD dwInstance);


//
// Utility functions
//

TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType);
CSize GetWindowSize(CWnd *pWnd);
void SelectListViewItem(CListCtrl *pListCtrl, int iItem);
const TCHAR *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText = _T("Unknown"));
TCHAR *TranslateUNIXText(const TCHAR *pszText);
void RestoreMDIChildPlacement(CMDIChildWnd *pWnd, WINDOWPLACEMENT *pwp);
BOOL IsButtonChecked(CDialog *pWnd, int nCtrl);
BOOL ExtractWindowParam(TCHAR *pszStr, TCHAR *pszParam, TCHAR *pszBuffer, int iSize);
long ExtractWindowParamLong(TCHAR *pszStr, TCHAR *pszParam, long nDefault);
DWORD ExtractWindowParamULong(TCHAR *pszStr, TCHAR *pszParam, DWORD dwDefault);
void CopyMenuItems(CMenu *pDst, CMenu *pSrc);
TCHAR **CopyStringList(TCHAR **ppList, DWORD dwSize);
void DestroyStringList(TCHAR **ppList, DWORD dwSize);
HTREEITEM FindTreeCtrlItem(CTreeCtrl &ctrl, HTREEITEM hRoot, TCHAR *pszText);
HTREEITEM FindTreeCtrlItemEx(CTreeCtrl &ctrl, HTREEITEM hRoot, DWORD dwData);
void SaveListCtrlColumns(CListCtrl &wndListCtrl, TCHAR *pszSection, TCHAR *pszPrefix);
void LoadListCtrlColumns(CListCtrl &wndListCtrl, TCHAR *pszSection, TCHAR *pszPrefix);
HGLOBAL CopyGlobalMem(HGLOBAL hSrc);


//
// Object tools functions
//

DWORD LoadObjectTools(void);
CMenu *CreateToolsSubmenu(NXC_OBJECT *pObject, TCHAR *pszCurrPath, DWORD *pdwStart);


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
extern DWORD g_dwMaxLogRecords;
extern TCHAR g_szServer[];
extern TCHAR g_szLogin[];
extern TCHAR g_szPassword[];
extern TCHAR g_szUpgradeURL[];
extern TCHAR g_szWorkDir[];
extern TCHAR *g_szStatusText[];
extern TCHAR *g_szStatusTextSmall[];
extern TCHAR *g_szAlarmState[];
extern TCHAR *g_szActionType[];
extern TCHAR *g_szServiceType[];
extern TCHAR *g_szDeploymentStatus[];
extern TCHAR *g_szToolType[];
extern TCHAR *g_szToolColFmt[];
extern TCHAR *g_szSyslogSeverity[];
extern TCHAR *g_szSyslogFacility[];
extern TCHAR *g_szAuthMethod[];
extern COLORREF g_statusColorTable[];
extern TCHAR *g_szObjectClass[];
extern TCHAR *g_szInterfaceTypes[];
extern TCHAR *g_pszItemOrigin[];
extern TCHAR *g_pszItemOriginLong[];
extern TCHAR *g_pszItemDataType[];
extern TCHAR *g_pszItemStatus[];
extern TCHAR *g_pszThresholdOperation[];
extern TCHAR *g_pszThresholdOperationLong[];
extern TCHAR *g_pszThresholdFunction[];
extern TCHAR *g_pszThresholdFunctionLong[];
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
extern DWORD g_dwNumObjectTools;
extern NXC_OBJECT_TOOL *g_pObjectToolList;
extern SNMP_MIBObject *g_pMIBRoot;
extern ALARM_SOUND_CFG g_soundCfg;
extern char g_szConfigKeywords[];
extern char g_szScriptKeywords[];


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


//
// Utility inline functions
//

inline void SafeGlobalFree(HGLOBAL hMem)
{
   if (hMem != NULL)
      GlobalFree(hMem);
}


#endif
