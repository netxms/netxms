#if !defined(AFX_VIEWEDITOR_H__05F3512E_B415_46B9_A7ED_B1BD9684AD2F__INCLUDED_)
#define AFX_VIEWEDITOR_H__05F3512E_B415_46B9_A7ED_B1BD9684AD2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewEditor.h : header file
//


//
// View element
//

class ViewElement
{
public:
   POINT m_pos;
   SIZE m_size;

   ViewElement() {};
   ~ViewElement() {};
};


/////////////////////////////////////////////////////////////////////////////
// CViewEditor frame

class CViewEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CViewEditor)
protected:
	CViewEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void DrawElement(CDC &dc, RECT &clRect, ViewElement *pElement);
	virtual ~CViewEditor();

	// Generated message map functions
	//{{AFX_MSG(CViewEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWEDITOR_H__05F3512E_B415_46B9_A7ED_B1BD9684AD2F__INCLUDED_)
