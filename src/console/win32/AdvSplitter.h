#if !defined(AFX_ADVSPLITTER_H__4E61C5B7_59F2_4DF7_9478_03D76A4CB5D8__INCLUDED_)
#define AFX_ADVSPLITTER_H__4E61C5B7_59F2_4DF7_9478_03D76A4CB5D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AdvSplitter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAdvSplitter frame with splitter

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CAdvSplitter : public CSplitterWnd
{
public:
	CAdvSplitter();

// Attributes
protected:
	BOOL m_bInitialized;
public:

// Operations
public:

virtual void AssertValid( ) const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvSplitter)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	void ShowColumn(int nCol, ...);
	void ShowRow(int nRow, ...);
	void HideColumn(int nCol);
	void HideRow(int nRow);
	void InitComplete(void);
	void SetupView(int nRow, int nCol, SIZE sizeInit);
	virtual ~CAdvSplitter();

	// Generated message map functions
	//{{AFX_MSG(CAdvSplitter)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADVSPLITTER_H__4E61C5B7_59F2_4DF7_9478_03D76A4CB5D8__INCLUDED_)
