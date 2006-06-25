// CondPropsScript.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondPropsScript.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCondPropsScript property page

IMPLEMENT_DYNCREATE(CCondPropsScript, CPropertyPage)

CCondPropsScript::CCondPropsScript() : CPropertyPage(CCondPropsScript::IDD)
{
	//{{AFX_DATA_INIT(CCondPropsScript)
	//}}AFX_DATA_INIT
}

CCondPropsScript::~CCondPropsScript()
{
   m_wndEditor.Detach();
}

void CCondPropsScript::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCondPropsScript)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCondPropsScript, CPropertyPage)
	//{{AFX_MSG_MAP(CCondPropsScript)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCondPropsScript message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCondPropsScript::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_wndEditor.Attach(::GetDlgItem(m_hWnd, IDC_EDIT_SCRIPT));
   m_wndEditor.SetDefaults();
   m_wndEditor.SetText((LPCTSTR)m_strScript);
	
	return TRUE;
}


//
// Handler for PSN_OK message
//

void CCondPropsScript::OnOK() 
{
   NXC_OBJECT_UPDATE *pUpdate;

	CPropertyPage::OnOK();
   if (m_wndEditor.GetModify())
   {
      pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
      pUpdate->dwFlags |= OBJ_UPDATE_SCRIPT;
      m_wndEditor.GetText(m_strScript);
      pUpdate->pszScript = (TCHAR *)((LPCTSTR)m_strScript);
   }
}
