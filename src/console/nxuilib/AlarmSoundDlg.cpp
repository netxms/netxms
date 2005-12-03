// AlarmSoundDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "AlarmSoundDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlarmSoundDlg dialog


CAlarmSoundDlg::CAlarmSoundDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAlarmSoundDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAlarmSoundDlg)
	m_iSoundType = -1;
	m_strSound1 = _T("");
	m_strSound2 = _T("");
	m_bIncludeSource = FALSE;
	m_bIncludeSeverity = FALSE;
	m_bIncludeMessage = FALSE;
	m_bNotifyOnAck = FALSE;
	//}}AFX_DATA_INIT
}


void CAlarmSoundDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAlarmSoundDlg)
	DDX_Radio(pDX, IDC_RADIO_NO_SOUND, m_iSoundType);
	DDX_CBString(pDX, IDC_COMBO_SOUND1, m_strSound1);
	DDV_MaxChars(pDX, m_strSound1, 255);
	DDX_CBString(pDX, IDC_COMBO_SOUND2, m_strSound2);
	DDV_MaxChars(pDX, m_strSound2, 255);
	DDX_Check(pDX, IDC_CHECK_SOURCE, m_bIncludeSource);
	DDX_Check(pDX, IDC_CHECK_SEVERITY, m_bIncludeSeverity);
	DDX_Check(pDX, IDC_CHECK_MESSAGE, m_bIncludeMessage);
	DDX_Check(pDX, IDC_CHECK_ACK, m_bNotifyOnAck);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAlarmSoundDlg, CDialog)
	//{{AFX_MSG_MAP(CAlarmSoundDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlarmSoundDlg message handlers
