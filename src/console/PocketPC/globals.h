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

class CNxpcApp;
extern CNxpcApp theApp;


//
// Constants
//

#define MAX_SERVER_NAME_LEN   64
#define MAX_LOGIN_NAME_LEN    64
#define MAX_PASSWORD_LEN      64
#define MAX_INTERFACE_TYPE    32


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
// Communication functions
//

DWORD DoLogin(void);
DWORD DoRequest(DWORD (* pFunc)(void), TCHAR *pszInfoText);
DWORD DoRequestArg1(void *pFunc, void *pArg1, TCHAR *pszInfoText);
DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, TCHAR *pszInfoText);
DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, TCHAR *pszInfoText);
DWORD DoRequestArg4(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, TCHAR *pszInfoText);
DWORD DoRequestArg5(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, TCHAR *pszInfoText);
DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, TCHAR *pszInfoText);
DWORD DoRequestArg7(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, TCHAR *pszInfoText);


//
// Drawing functions
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors);
//void Draw3dRect(HDC hDC, LPRECT pRect, COLORREF rgbTop, COLORREF rgbBottom);


//
// Image and image list functions
//

CImageList *CreateEventImageList(void);
void LoadBitmapIntoList(CImageList *pImageList, UINT nIDResource, COLORREF rgbMaskColor);


//
// Utility functions
//

int MulDiv(int nNumber, int nNumerator, int nDenominator);
/*TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType);
CSize GetWindowSize(CWnd *pWnd);
void SelectListViewItem(CListCtrl *pListCtrl, int iItem);
const TCHAR *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText = "Unknown");
TCHAR *TranslateUNIXText(const TCHAR *pszText);
void RestoreMDIChildPlacement(CMDIChildWnd *pWnd, WINDOWPLACEMENT *pwp);
void EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable);*/


//
// Variables
//

extern NXC_SESSION g_hSession;
extern DWORD g_dwOptions;
extern TCHAR g_szServer[];
extern TCHAR g_szLogin[];
extern TCHAR g_szPassword[];
extern DWORD g_dwEncryptionMethod;
extern TCHAR g_szWorkDir[];
extern TCHAR *g_szStatusText[];
extern TCHAR *g_szStatusTextSmall[];
extern TCHAR *g_szActionType[];
extern TCHAR *g_szServiceType[];
extern COLORREF g_statusColorTable[];
extern TCHAR *g_szObjectClass[];
extern TCHAR *g_szInterfaceTypes[];
extern TCHAR *g_pszItemOrigin[];
extern TCHAR *g_pszItemOriginLong[];
extern TCHAR *g_pszItemDataType[];
extern TCHAR *g_pszItemStatus[];
extern NXC_CC_LIST *g_pCCList;
extern CODE_TO_TEXT g_ctNodeType[];

#endif
