#if !defined(AFX_OBJECTPROPSTRUSTEDNODES_H__CFD8DF96_EE68_44E1_93CE_EB113771FAA2__INCLUDED_)
#define AFX_OBJECTPROPSTRUSTEDNODES_H__CFD8DF96_EE68_44E1_93CE_EB113771FAA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsTrustedNodes.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsTrustedNodes dialog

class CObjectPropsTrustedNodes : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsTrustedNodes)

// Construction
public:
	UINT32 * m_pdwNodeList;
	UINT32 m_dwNumNodes;
	CObjectPropsTrustedNodes();
	~CObjectPropsTrustedNodes();

// Dialog Data
	//{{AFX_DATA(CObjectPropsTrustedNodes)
	enum { IDD = IDD_OBJECT_TRUSTED_NODES };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsTrustedNodes)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsTrustedNodes)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSTRUSTEDNODES_H__CFD8DF96_EE68_44E1_93CE_EB113771FAA2__INCLUDED_)
