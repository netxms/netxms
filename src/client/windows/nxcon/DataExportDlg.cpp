// DataExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataExportDlg dialog


CDataExportDlg::CDataExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDataExportDlg::IDD, pParent)
{
   time_t currTime;

	//{{AFX_DATA_INIT(CDataExportDlg)
	m_iTimeStampFormat = -1;
	m_iSeparator = -1;
	m_strFileName = _T("");
	m_dateFrom = 0;
	m_dateTo = 0;
	m_timeFrom = 0;
	m_timeTo = 0;
	//}}AFX_DATA_INIT

   currTime = time(NULL);

   m_iTimeStampFormat = theApp.GetProfileInt(_T("DataExport"), _T("TimeStampFormat"), 1);
   m_iSeparator = theApp.GetProfileInt(_T("DataExport"), _T("Separator"), 0);
   m_strFileName = theApp.GetProfileString(_T("DataExport"), _T("FileName"), NULL);
   m_dateFrom = theApp.GetProfileInt(_T("DataExport"), _T("TimeFrom"), (int)(currTime - 86400));
   m_timeFrom = theApp.GetProfileInt(_T("DataExport"), _T("TimeFrom"), (int)(currTime - 86400));
   m_dateTo = theApp.GetProfileInt(_T("DataExport"), _T("TimeTo"), (int)currTime);
   m_timeTo = theApp.GetProfileInt(_T("DataExport"), _T("TimeTo"), (int)currTime);
}


void CDataExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDataExportDlg)
	DDX_Radio(pDX, IDC_RADIO_RAW, m_iTimeStampFormat);
	DDX_Radio(pDX, IDC_RADIO_TAB, m_iSeparator);
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFileName);
	DDV_MaxChars(pDX, m_strFileName, 255);
	DDX_DateTimeCtrl(pDX, IDC_DATE_FROM, m_dateFrom);
	DDX_DateTimeCtrl(pDX, IDC_DATE_TO, m_dateTo);
	DDX_DateTimeCtrl(pDX, IDC_TIME_FROM, m_timeFrom);
	DDX_DateTimeCtrl(pDX, IDC_TIME_TO, m_timeTo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDataExportDlg, CDialog)
	//{{AFX_MSG_MAP(CDataExportDlg)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataExportDlg message handlers

void CDataExportDlg::SaveLastSelection()
{
   DWORD dwTimeTo, dwTimeFrom;

   theApp.WriteProfileInt(_T("DataExport"), _T("TimeStampFormat"), m_iTimeStampFormat);
   theApp.WriteProfileInt(_T("DataExport"), _T("Separator"), m_iSeparator);
   theApp.WriteProfileString(_T("DataExport"), _T("FileName"), (LPCTSTR)m_strFileName);

   dwTimeFrom = (DWORD)CTime(m_dateFrom.GetYear(), m_dateFrom.GetMonth(),
                      m_dateFrom.GetDay(), m_timeFrom.GetHour(),
                      m_timeFrom.GetMinute(),
                      m_timeFrom.GetSecond(), -1).GetTime();
   dwTimeTo = (DWORD)CTime(m_dateTo.GetYear(), m_dateTo.GetMonth(),
                    m_dateTo.GetDay(), m_timeTo.GetHour(),
                    m_timeTo.GetMinute(),
                    m_timeTo.GetSecond(), -1).GetTime();
   theApp.WriteProfileInt(_T("DataExport"), _T("TimeFrom"), dwTimeFrom);
   theApp.WriteProfileInt(_T("DataExport"), _T("TimeTo"), dwTimeTo);
}


//
// Handler for "Browse" button
//

void CDataExportDlg::OnBrowse() 
{
   TCHAR szBuffer[MAX_PATH];

   GetDlgItemText(IDC_EDIT_FILE, szBuffer, MAX_PATH);
   CFileDialog dlg(FALSE, _T(".txt"), szBuffer);
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
