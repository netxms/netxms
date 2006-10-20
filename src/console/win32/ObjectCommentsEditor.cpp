// ObjectCommentsEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectCommentsEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectCommentsEditor

IMPLEMENT_DYNCREATE(CObjectCommentsEditor, CMDIChildWnd)

CObjectCommentsEditor::CObjectCommentsEditor()
{
   m_dwObjectId = 0;
}

CObjectCommentsEditor::CObjectCommentsEditor(DWORD dwObjectId)
{
   m_dwObjectId = dwObjectId;
}

CObjectCommentsEditor::~CObjectCommentsEditor()
{
}


BEGIN_MESSAGE_MAP(CObjectCommentsEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CObjectCommentsEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectCommentsEditor message handlers

BOOL CObjectCommentsEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectCommentsEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndEdit.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE, rect, this, ID_EDIT_CTRL);

   theApp.OnViewCreate(OV_OBJECT_COMMENTS, this, m_dwObjectId);
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CObjectCommentsEditor::OnDestroy() 
{
   theApp.OnViewDestroy(OV_OBJECT_COMMENTS, this, m_dwObjectId);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CObjectCommentsEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndEdit.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CObjectCommentsEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndEdit.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CObjectCommentsEditor::OnViewRefresh() 
{
   DWORD dwResult;
   TCHAR *pszText;

   dwResult = NXCGetObjectComments(g_hSession, m_dwObjectId, &pszText);
   if (dwResult == RCC_SUCCESS)
   {
      if (pszText != NULL)
      {
         m_wndEdit.SetWindowText(pszText);
         free(pszText);
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot get object's comments: %s"));
   }
}
