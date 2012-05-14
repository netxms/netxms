#if !defined(AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_)
#define AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropSheet

class CObjectPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CObjectPropSheet)

// Construction
public:
	CObjectPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CObjectPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropSheet)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectPropSheet();

   NXC_OBJECT_UPDATE *GetUpdateStruct() { return &m_update; }
   void SetObject(NXC_OBJECT *pObject) { if (pObject != NULL) m_update.dwObjectId = pObject->dwId; }
	void SaveObjectChanges();

	// Generated message map functions
protected:
	NXC_OBJECT_UPDATE m_update;
	//{{AFX_MSG(CObjectPropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_)
