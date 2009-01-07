// InputBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInputBox dialog


CInputBox::CInputBox(CWnd* pParent /*=NULL*/)
	: CDialog(CInputBox::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInputBox)
	m_strText = _T("");
	m_strHeader = _T("");
	//}}AFX_DATA_INIT

   m_strTitle = _T("Input data");
   m_strHeader = _T("Enter data:");
   m_strErrorMsg = _T("Invalid text entered");
   m_pfTextValidator = NULL;
}


void CInputBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInputBox)
	DDX_Text(pDX, IDC_EDIT_TEXT, m_strText);
	DDX_Text(pDX, IDC_STATIC_HEADER, m_strHeader);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInputBox, CDialog)
	//{{AFX_MSG_MAP(CInputBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInputBox message handlers

BOOL CInputBox::OnInitDialog() 
{
	CDialog::OnInitDialog();
   SetWindowText(m_strTitle);
	return TRUE;
}


//
// "OK" button handler
//

void CInputBox::OnOK() 
{
   if (m_pfTextValidator != NULL)
   {
      CString strText;

      GetDlgItemText(IDC_EDIT_TEXT, strText);
      if (m_pfTextValidator((LPCTSTR)strText))
      {
      	CDialog::OnOK();
      }
      else
      {
         MessageBox(m_strErrorMsg, _T("Error"), MB_OK | MB_ICONSTOP);
         ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_TEXT));
      }
   }
   else
   {
	   CDialog::OnOK();
   }
}

