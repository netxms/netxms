// CreateTemplateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateTemplateDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateTemplateDlg dialog


CCreateTemplateDlg::CCreateTemplateDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateTemplateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateTemplateDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCreateTemplateDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateTemplateDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateTemplateDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateTemplateDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateTemplateDlg message handlers
