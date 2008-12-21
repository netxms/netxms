// SubmapBkgndDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SubmapBkgndDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSubmapBkgndDlg dialog


CSubmapBkgndDlg::CSubmapBkgndDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSubmapBkgndDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSubmapBkgndDlg)
	m_strFileName = _T("");
	m_nScaleMethod = -1;
	m_nBkType = -1;
	//}}AFX_DATA_INIT
}


void CSubmapBkgndDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSubmapBkgndDlg)
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFileName);
	DDV_MaxChars(pDX, m_strFileName, 255);
	DDX_Radio(pDX, IDC_RADIO_NO_SCALE, m_nScaleMethod);
	DDX_Radio(pDX, IDC_RADIO_NBK, m_nBkType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSubmapBkgndDlg, CDialog)
	//{{AFX_MSG_MAP(CSubmapBkgndDlg)
	ON_BN_CLICKED(IDC_RADIO_BITMAP, OnRadioBitmap)
	ON_BN_CLICKED(IDC_RADIO_NBK, OnRadioNbk)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubmapBkgndDlg message handlers

BOOL CSubmapBkgndDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   EnableItems();
	
	return TRUE;
}


//
// Enable/disable items based on current selection
//

void CSubmapBkgndDlg::EnableItems()
{
   BOOL bEnable;

   bEnable = IsButtonChecked(this, IDC_RADIO_BITMAP);
   EnableDlgItem(this, IDC_STATIC_FRAME, bEnable);
   EnableDlgItem(this, IDC_STATIC_FILE, bEnable);
   EnableDlgItem(this, IDC_EDIT_FILE, bEnable);
   EnableDlgItem(this, IDC_BROWSE, bEnable);
   EnableDlgItem(this, IDC_RADIO_NO_SCALE, bEnable);
   EnableDlgItem(this, IDC_RADIO_SCALE, bEnable);
}


//
// Handlers for radio buttons
//

void CSubmapBkgndDlg::OnRadioBitmap() 
{
   EnableItems();
}

void CSubmapBkgndDlg::OnRadioNbk() 
{
   EnableItems();
}


//
// "Browse" button handler
//

void CSubmapBkgndDlg::OnBrowse() 
{
   TCHAR szBuffer[MAX_PATH];

   GetDlgItemText(IDC_EDIT_FILE, szBuffer, MAX_PATH);
   CFileDialog dlg(TRUE, NULL, szBuffer, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                   _T("Image Files (*.jpg;*.jpeg;*.gif;*.bmp)|*.jpg;*.jpeg;*.gif;*.bmp|All Files (*.*)|*.*||"));
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
