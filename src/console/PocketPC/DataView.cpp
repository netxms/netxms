// DataView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "DataView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataView

CDataView::CDataView()
{
}

CDataView::~CDataView()
{
}


BEGIN_MESSAGE_MAP(CDataView, CDynamicView)
	//{{AFX_MSG_MAP(CDataView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Get view's fingerprint
// Should be overriden in child classes.
//

QWORD CDataView::GetFingerprint()
{
   return CREATE_VIEW_FINGERPRINT(VIEW_CLASS_LAST_VALUES, 0, m_dwNodeId);
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Data is a pointer to object.
//

void CDataView::InitView(void *pData)
{
   //m_dwNodeId = ((NXC_OBJECT *)pData)->dwId;
}


/////////////////////////////////////////////////////////////////////////////
// CDataView message handlers
