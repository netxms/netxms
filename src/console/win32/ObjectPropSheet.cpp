// ObjectPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropSheet

IMPLEMENT_DYNAMIC(CObjectPropSheet, CPropertySheet)

CObjectPropSheet::CObjectPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CObjectPropSheet::CObjectPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CObjectPropSheet::~CObjectPropSheet()
{
}


BEGIN_MESSAGE_MAP(CObjectPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CObjectPropSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropSheet message handlers
