// DCIPerfTabPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIPerfTabPage.h"
#include <nxconfig.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCIPerfTabPage property page

IMPLEMENT_DYNCREATE(CDCIPerfTabPage, CPropertyPage)

CDCIPerfTabPage::CDCIPerfTabPage() : CPropertyPage(CDCIPerfTabPage::IDD)
{
	//{{AFX_DATA_INIT(CDCIPerfTabPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_showOnPerfTab = false;
	m_graphTitle = m_dciDescription;
	m_graphColor = RGB(0, 192, 0);
}

CDCIPerfTabPage::~CDCIPerfTabPage()
{
}

void CDCIPerfTabPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIPerfTabPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_COLOR_SELECTOR, m_wndGraphColor);
	DDX_ColourPickerXP(pDX, IDC_COLOR_SELECTOR, m_graphColor);

	if (pDX->m_bSaveAndValidate)
	{
		m_showOnPerfTab = (SendDlgItemMessage(IDC_CHECK_SHOW, BM_GETCHECK, 0, 0) == BST_CHECKED);
		m_showThresholds = (SendDlgItemMessage(IDC_CHECK_THRESHOLDS, BM_GETCHECK, 0, 0) == BST_CHECKED);
		GetDlgItemText(IDC_EDIT_TITLE, m_graphTitle);

		// Check for default settings
		if (!m_showOnPerfTab && (m_graphColor == RGB(0, 192, 0)) &&
		    (m_graphTitle.IsEmpty() || (m_graphTitle.CompareNoCase(m_dciDescription) == 0)))
		{
			m_strConfig = _T("");
		}
		else
		{
			TCHAR temp[64];

			m_strConfig = _T("<config><enabled>");
			m_strConfig += m_showOnPerfTab ? _T("true") : _T("false");
			m_strConfig += _T("</enabled><title>");
			m_strConfig += (const TCHAR *)EscapeStringForXML2((LPCTSTR)m_graphTitle, -1);
			m_strConfig += _T("</title><color>");
			_sntprintf(temp, 64, _T("0x%06X</color><showThresholds>"), m_graphColor);
			m_strConfig += temp;
			m_strConfig += m_showThresholds ? _T("true") : _T("false");
			m_strConfig += _T("</showThresholds></config>");
		}
	}
}


BEGIN_MESSAGE_MAP(CDCIPerfTabPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCIPerfTabPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCIPerfTabPage message handlers

BOOL CDCIPerfTabPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	if (!m_strConfig.IsEmpty())
	{
		Config config;
		char *xml;

#ifdef UNICODE
		xml = UTF8StringFromWideString(m_strConfig);
#else
		xml = strdup(m_strConfig);
#endif
		if (config.loadXmlConfigFromMemory(xml, (int)strlen(xml)))
		{
			m_showOnPerfTab = config.getValueAsBoolean(_T("/enabled"), false);
			m_graphTitle = config.getValue(_T("/title"), _T(""));
			m_graphColor = config.getValueAsUInt(_T("/color"), 0x00C000);
			m_showThresholds = config.getValueAsBoolean(_T("/showThresholds"), false);

			SendDlgItemMessage(IDC_CHECK_SHOW, BM_SETCHECK, m_showOnPerfTab ? BST_CHECKED : BST_UNCHECKED, 0);
			SetDlgItemText(IDC_EDIT_TITLE, m_graphTitle);
			m_wndGraphColor.SetColor(m_graphColor);
			SendDlgItemMessage(IDC_CHECK_THRESHOLDS, BM_SETCHECK, m_showThresholds ? BST_CHECKED : BST_UNCHECKED, 0);
		}
		free(xml);
	}

	return TRUE;
}
