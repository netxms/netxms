// EventEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventEditor.h"
#include "EditEventDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventEditor

IMPLEMENT_DYNCREATE(CEventEditor, CMDIChildWnd)

CEventEditor::CEventEditor()
{
   m_bModified = FALSE;
}

CEventEditor::~CEventEditor()
{
}


BEGIN_MESSAGE_MAP(CEventEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
   ON_NOTIFY(NM_DBLCLK, ID_LIST_VIEW, OnListViewDoubleClick)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// CEventEditor message handlers
//

int CEventEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   DWORD dwResult;
   char szBuffer[256];

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, "Severity", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "Flags", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(4, "Message", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(5, "Description", LVCFMT_LEFT, 300);
	
   theApp.OnViewCreate(IDR_EVENT_EDITOR, this);

   dwResult = theApp.WaitForRequest(NXCOpenEventDB(), "Opening event configuration database...");
   if (dwResult == RCC_SUCCESS)
   {
      DWORD i;

      NXCGetEventDB(&m_ppEventTemplates, &m_dwNumTemplates);
      for(i = 0; i < m_dwNumTemplates; i++)
      {
         sprintf(szBuffer, "%ld", m_ppEventTemplates[i]->dwCode);
         m_wndListCtrl.InsertItem(i, szBuffer);
         m_wndListCtrl.SetItemText(i, 1, m_ppEventTemplates[i]->szName);
         sprintf(szBuffer, "%ld", m_ppEventTemplates[i]->dwSeverity);
         m_wndListCtrl.SetItemText(i, 2, szBuffer);
         sprintf(szBuffer, "%ld", m_ppEventTemplates[i]->dwFlags);
         m_wndListCtrl.SetItemText(i, 3, szBuffer);
         m_wndListCtrl.SetItemText(i, 4, m_ppEventTemplates[i]->pszMessage);
         m_wndListCtrl.SetItemText(i, 5, m_ppEventTemplates[i]->pszDescription);
         m_wndListCtrl.SetItemData(i, i);
      }
   }
   else
   {
      sprintf(szBuffer, "Unable to open event configuration database:\n%s", NXCGetErrorText(dwResult));
      theApp.GetMainWnd()->MessageBox(szBuffer, "Error", MB_ICONSTOP);

      // For unknown reason MFC crashes if this function just returns -1,
      // so we just post WM_CLOSE to itself
      PostMessage(WM_CLOSE, 0, 0);
   }

   return 0;
}


//
// WM_DESTROY message handler
//

void CEventEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_EVENT_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CEventEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Handler for double-clicks in list view
//

void CEventEditor::OnListViewDoubleClick(NMITEMACTIVATE *pInfo, LRESULT *pResult)
{
   if (pInfo->iItem != -1)
      EditEvent(pInfo->iItem);
}


//
// Edit currently selected event
//

void CEventEditor::EditEvent(int iItem)
{
   int iIdx;
   DWORD dwFlags;
   CEditEventDlg dlgEditEvent;

   iIdx = m_wndListCtrl.GetItemData(iItem);

   dlgEditEvent.m_bWriteLog = m_ppEventTemplates[iIdx]->dwFlags & EF_LOG;
   dlgEditEvent.m_dwEventId = m_ppEventTemplates[iIdx]->dwCode;
   dlgEditEvent.m_strName = m_ppEventTemplates[iIdx]->szName;
   dlgEditEvent.m_strMessage = m_ppEventTemplates[iIdx]->pszMessage;
   dlgEditEvent.m_strDescription = m_ppEventTemplates[iIdx]->pszDescription;
   dlgEditEvent.m_dwSeverity = m_ppEventTemplates[iIdx]->dwSeverity;

   if (dlgEditEvent.DoModal() == IDOK)
   {
      dwFlags = 0;
      if (dlgEditEvent.m_bWriteLog)
         dwFlags |= EF_LOG;
      NXCModifyEventTemplate(m_ppEventTemplates[iIdx], EM_ALL, dlgEditEvent.m_dwSeverity,
         dwFlags, (LPCTSTR)dlgEditEvent.m_strName, (LPCTSTR)dlgEditEvent.m_strMessage,
         (LPCTSTR)dlgEditEvent.m_strDescription);
      m_bModified = TRUE;
   }
}


//
// WM_SETFOCUS message handler
//

void CEventEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_CLOSE message handler
//

void CEventEditor::OnClose() 
{
   static char szClosingMessage[] = "Closing event configuration database...";
   int iAction;
   DWORD dwResult;

   if (m_bModified)
   {
      iAction = MessageBox("Event configuration database has been modified. "
                           "Do you wish to save it?", "Event Editor", 
                           MB_YESNOCANCEL | MB_ICONQUESTION);
      switch(iAction)
      {
         case IDYES:
            dwResult = theApp.WaitForRequest(NXCCloseEventDB(TRUE), szClosingMessage);
            if (dwResult == RCC_SUCCESS)
            {
         	   CMDIChildWnd::OnClose();
            }
            else
            {
               char szBuffer[256];

               sprintf(szBuffer, "Error closing event configuration database:\n%s",
                       NXCGetErrorText(dwResult));
               MessageBox(szBuffer, "Event Editor", MB_OK | MB_ICONSTOP);
            }
            break;
         case IDNO:
            theApp.WaitForRequest(NXCCloseEventDB(FALSE), szClosingMessage);
      	   CMDIChildWnd::OnClose();
            break;
         case IDCANCEL:
            break;
      }
   }
   else
   {
      theApp.WaitForRequest(NXCCloseEventDB(FALSE), szClosingMessage);
	   CMDIChildWnd::OnClose();
   }
}
