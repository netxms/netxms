// MapControlBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapControlBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapControlBox

CMapControlBox::CMapControlBox()
               :CToolBox()
{
}

CMapControlBox::~CMapControlBox()
{
}


BEGIN_MESSAGE_MAP(CMapControlBox, CToolBox)
	//{{AFX_MSG_MAP(CMapControlBox)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGING, ID_LIST_VIEW, OnListViewItemChanging)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMapControlBox message handlers

int CMapControlBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CToolBox::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_font.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                     0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     VARIABLE_PITCH | FF_DONTCARE, "Verdana");

   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | 
                                  LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetFont(&m_font, FALSE);
   m_wndListCtrl.InsertColumn(0, _T("Action"));

   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ZOOMIN));
   m_imageList.Add(theApp.LoadIcon(IDI_ZOOMOUT));
   m_imageList.Add(theApp.LoadIcon(IDI_SAVE));
   m_imageList.Add(theApp.LoadIcon(IDI_REFRESH));
   m_imageList.Add(theApp.LoadIcon(IDI_PRINT));
   m_imageList.Add(theApp.LoadIcon(IDI_BACK));
   m_imageList.Add(theApp.LoadIcon(IDI_FORWARD));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
   
   AddCommand(_T("Back"), 5, ID_MAP_BACK);
   AddCommand(_T("Forward"), 6, ID_MAP_FORWARD);
   AddCommand(_T("Home"), 0, ID_MAP_HOME);
   AddCommand(_T(""), -1, 0);
   AddCommand(_T("Refresh"), 3, ID_VIEW_REFRESH);
   AddCommand(_T(""), -1, 0);
   AddCommand(_T("Zoom in"), 0, ID_MAP_ZOOMIN);
   AddCommand(_T("Zoom out"), 1, ID_MAP_ZOOMOUT);
   AddCommand(_T(""), -1, 0);
   AddCommand(_T("Save changes"), 2, ID_MAP_SAVE);
   AddCommand(_T("Print"), 4, 0);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CMapControlBox::OnSize(UINT nType, int cx, int cy) 
{
	CToolBox::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 2, 2, cx - 4, cy - 4, SWP_NOZORDER);
   m_wndListCtrl.SetColumnWidth(0, cx - 4);
}


//
// WM_NOTIFY::LVN_ITEMCHANGING message handler
//

void CMapControlBox::OnListViewItemChanging(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   *pResult = FALSE;
   if (pNMHDR->iItem != -1)
   {
      if (pNMHDR->uChanged & LVIF_STATE)
      {
         if (pNMHDR->uNewState & LVIS_FOCUSED)
         {
            GetParent()->PostMessage(WM_COMMAND, m_wndListCtrl.GetItemData(pNMHDR->iItem), 0);
            *pResult = TRUE;
         }
      }
   }
}


//
// Add new command to control box
//

void CMapControlBox::AddCommand(TCHAR *pszText, int nImage, int nCommand)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszText, nImage);
   if (iItem != -1)
      m_wndListCtrl.SetItemData(iItem, nCommand);
}
