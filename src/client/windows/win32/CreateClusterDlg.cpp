// CreateClusterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateClusterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateClusterDlg dialog


CCreateClusterDlg::CCreateClusterDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateClusterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateClusterDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCreateClusterDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateClusterDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateClusterDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateClusterDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateClusterDlg message handlers
