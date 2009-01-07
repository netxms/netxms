// CreateCondDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateCondDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateCondDlg dialog


CCreateCondDlg::CCreateCondDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateCondDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateCondDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCreateCondDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateCondDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateCondDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateCondDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateCondDlg message handlers
