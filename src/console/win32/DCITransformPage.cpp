// DCITransformPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCITransformPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Delta calculation methods
//

static char *m_pszMethodList[] =
{
   "None - Keep original value",
   "Simple delta",
   "Average delta per second",
   "Average delta per minute",
   NULL
};


/////////////////////////////////////////////////////////////////////////////
// CDCITransformPage property page

IMPLEMENT_DYNCREATE(CDCITransformPage, CPropertyPage)

CDCITransformPage::CDCITransformPage() : CPropertyPage(CDCITransformPage::IDD)
{
	//{{AFX_DATA_INIT(CDCITransformPage)
	m_iDeltaProc = -1;
	m_strFormula = _T("");
	//}}AFX_DATA_INIT
}

CDCITransformPage::~CDCITransformPage()
{
}

void CDCITransformPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCITransformPage)
	DDX_Control(pDX, IDC_COMBO_DELTA, m_wndDeltaList);
	DDX_CBIndex(pDX, IDC_COMBO_DELTA, m_iDeltaProc);
	DDX_Text(pDX, IDC_EDIT_FORMULA, m_strFormula);
	DDV_MaxChars(pDX, m_strFormula, 1023);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCITransformPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCITransformPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCITransformPage message handlers

BOOL CDCITransformPage::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();

   // Fill in delta calculation methods list
   for(i = 0; m_pszMethodList[i] != NULL; i++)
      m_wndDeltaList.AddString(m_pszMethodList[i]);
   m_wndDeltaList.SelectString(-1, m_pszMethodList[m_iDeltaProc]);
	
	return TRUE;
}
