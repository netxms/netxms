// ObjectPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropSheet.h"

#ifndef IDAPPLY
#define IDAPPLY 12321
#endif

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
   memset(&m_update, 0, sizeof(NXC_OBJECT_UPDATE));
   m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

CObjectPropSheet::CObjectPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   memset(&m_update, 0, sizeof(NXC_OBJECT_UPDATE));
   m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

CObjectPropSheet::~CObjectPropSheet()
{
   // Access list can be allocated if we have changed it in "Security" tab
   // Otherwise it will be NULL because of memset() in constructor
   MemFree(m_update.pAccessList);
}


BEGIN_MESSAGE_MAP(CObjectPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CObjectPropSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropSheet message handlers


//
// Save changes to object
//

void CObjectPropSheet::SaveObjectChanges()
{
   DWORD dwResult;

   if (m_update.dwFlags != 0)
   {
      theApp.DebugPrintf("Saving changes for object %d (Flags: 0x%04X)", 
                         m_update.dwObjectId, m_update.dwFlags);
      dwResult = DoRequestArg1(NXCModifyObject, &m_update, "Updating object...");
      if (dwResult != RCC_SUCCESS)
      {
         char szBuffer[256];

         sprintf(szBuffer, "Error updating object: %s", NXCGetErrorText(dwResult));
         MessageBox(szBuffer, "Error", MB_OK | MB_ICONSTOP);
      }
   }
}
