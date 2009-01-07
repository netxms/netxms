// CreateVPNConnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateVPNConnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateVPNConnDlg dialog


CCreateVPNConnDlg::CCreateVPNConnDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateVPNConnDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateVPNConnDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCreateVPNConnDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateVPNConnDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateVPNConnDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateVPNConnDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateVPNConnDlg message handlers
