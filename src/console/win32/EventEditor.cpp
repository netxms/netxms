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
   m_pImageList = NULL;
}

CEventEditor::~CEventEditor()
{
   delete m_pImageList;
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
   char szBuffer[32];
   DWORD i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   theApp.OnViewCreate(IDR_EVENT_EDITOR, this);

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 120);
   m_wndListCtrl.InsertColumn(2, "Severity", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "Flags", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(4, "Message", LVCFMT_LEFT, 180);
   m_wndListCtrl.InsertColumn(5, "Description", LVCFMT_LEFT, 300);
	
   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Load event templates
   NXCGetEventDB(&m_ppEventTemplates, &m_dwNumTemplates);
   for(i = 0; i < m_dwNumTemplates; i++)
   {
      sprintf(szBuffer, "%ld", m_ppEventTemplates[i]->dwCode);
      m_wndListCtrl.InsertItem(i, szBuffer, m_ppEventTemplates[i]->dwSeverity);
      m_wndListCtrl.SetItemText(i, 1, m_ppEventTemplates[i]->szName);
      m_wndListCtrl.SetItemText(i, 2, g_szStatusTextSmall[m_ppEventTemplates[i]->dwSeverity]);
      sprintf(szBuffer, "%ld", m_ppEventTemplates[i]->dwFlags);
      m_wndListCtrl.SetItemText(i, 3, szBuffer);
      m_wndListCtrl.SetItemText(i, 4, m_ppEventTemplates[i]->pszMessage);
      m_wndListCtrl.SetItemText(i, 5, m_ppEventTemplates[i]->pszDescription);
      m_wndListCtrl.SetItemData(i, i);
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
            dwResult = DoRequestArg1(NXCCloseEventDB, (void *)TRUE, szClosingMessage);
            if (dwResult == RCC_SUCCESS)
            {
         	   CMDIChildWnd::OnClose();
            }
            else
            {
               theApp.ErrorBox(dwResult, "Error closing event configuration database:\n%s",
                               "Event Editor");
            }
            break;
         case IDNO:
            dwResult = DoRequestArg1(NXCCloseEventDB, (void *)FALSE, szClosingMessage);
      	   CMDIChildWnd::OnClose();
            break;
         case IDCANCEL:
            break;
      }
   }
   else
   {
      dwResult = DoRequestArg1(NXCCloseEventDB, (void *)FALSE, szClosingMessage);
	   CMDIChildWnd::OnClose();
   }
}
