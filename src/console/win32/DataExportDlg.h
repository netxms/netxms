#if !defined(AFX_DATAEXPORTDLG_H__BBC5347A_AFB8_48D4_AEAA_34031DC2B1D2__INCLUDED_)
#define AFX_DATAEXPORTDLG_H__BBC5347A_AFB8_48D4_AEAA_34031DC2B1D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataExportDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDataExportDlg dialog

class CDataExportDlg : public CDialog
{
// Construction
public:
	void SaveLastSelection(void);
	CDataExportDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDataExportDlg)
	enum { IDD = IDD_DCI_DATA_EXPORT };
	int		m_iTimeStampFormat;
	int		m_iSeparator;
	CString	m_strFileName;
	CTime	m_dateFrom;
	CTime	m_dateTo;
	CTime	m_timeFrom;
	CTime	m_timeTo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataExportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDataExportDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAEXPORTDLG_H__BBC5347A_AFB8_48D4_AEAA_34031DC2B1D2__INCLUDED_)
