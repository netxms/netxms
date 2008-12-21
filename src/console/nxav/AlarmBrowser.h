#if !defined(AFX_ALARMBROWSER_H__A2753A5B_AD4A_4FD5_B5FD_57231FF1EB5A__INCLUDED_)
#define AFX_ALARMBROWSER_H__A2753A5B_AD4A_4FD5_B5FD_57231FF1EB5A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser html view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
#include <afxhtml.h>

class CAlarmBrowser : public CHtmlView
{
protected:
	CAlarmBrowser();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAlarmBrowser)

// html Data
public:
	//{{AFX_DATA(CAlarmBrowser)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	BOOL SetHTML(CString &strHTML);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmBrowser)
	public:
	virtual void OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags, LPCTSTR lpszTargetFrameName, CByteArray& baPostedData, LPCTSTR lpszHeaders, BOOL* pbCancel);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL TerminateAlarm(DWORD dwAlarmId);
	BOOL AcknowledgeAlarm(DWORD dwAlarmId);
	virtual ~CAlarmBrowser();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CAlarmBrowser)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMBROWSER_H__A2753A5B_AD4A_4FD5_B5FD_57231FF1EB5A__INCLUDED_)
