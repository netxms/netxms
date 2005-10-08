// LPPList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "LPPList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLPPList

IMPLEMENT_DYNCREATE(CLPPList, CMDIChildWnd)

CLPPList::CLPPList()
{
}

CLPPList::~CLPPList()
{
}


BEGIN_MESSAGE_MAP(CLPPList, CMDIChildWnd)
	//{{AFX_MSG_MAP(CLPPList)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLPPList message handlers

BOOL CLPPList::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_LPP));
	return CMDIChildWnd::PreCreateWindow(cs);
}
