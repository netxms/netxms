#if !defined(AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_)
#define AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjToolPropColumns.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns dialog

class CObjToolPropColumns : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjToolPropColumns)

// Construction
public:
	int m_iToolType;
	CObjToolPropColumns();
	~CObjToolPropColumns();

// Dialog Data
	//{{AFX_DATA(CObjToolPropColumns)
	enum { IDD = IDD_OBJTOOL_COLUMNS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjToolPropColumns)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjToolPropColumns)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_)
