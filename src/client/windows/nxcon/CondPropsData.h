#if !defined(AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_)
#define AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CondPropsData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCondPropsData dialog

class CCondPropsData : public CPropertyPage
{
	DECLARE_DYNCREATE(CCondPropsData)

// Construction
public:
	NXC_OBJECT *m_pObject;
	CCondPropsData();
	~CCondPropsData();

// Dialog Data
	//{{AFX_DATA(CCondPropsData)
	enum { IDD = IDD_OBJECT_COND_DATA };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCondPropsData)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	NXC_OBJECT_UPDATE *m_pUpdate;
	// Generated message map functions
	//{{AFX_MSG(CCondPropsData)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	afx_msg void OnItemchangedListDci(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonEdit();
	afx_msg void OnDblclkListDci(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int AddListItem(int nPos, INPUT_DCI *pItem, TCHAR *pszName);
	INPUT_DCI *m_pDCIList;
	DWORD m_dwNumDCI;

   void Modify(void) { m_pUpdate->qwFlags |= OBJ_UPDATE_DCI_LIST; }
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_)
