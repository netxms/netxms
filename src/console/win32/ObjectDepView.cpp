// ObjectDepView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectDepView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectDepView

CObjectDepView::CObjectDepView()
{
   m_pObject = NULL;
}

CObjectDepView::~CObjectDepView()
{
}


BEGIN_MESSAGE_MAP(CObjectDepView, CWnd)
	//{{AFX_MSG_MAP(CObjectDepView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectDepView message handlers


//
// WM_CREATE message handler
//

int CObjectDepView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndMap.CreateEx(0, NULL, _T("Map"), WS_CHILD | WS_VISIBLE, rect, this, 0);
   m_wndMap.m_rgbBkColor = RGB(255, 255, 255);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectDepView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
   m_wndMap.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXCM_SET_OBJECT message handler
//

void CObjectDepView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
   Refresh();
}


//
// Refresh view
//

void CObjectDepView::Refresh()
{
   nxMap *pMap;
   nxSubmap *pSubmap;
   nxObjList list;
   RECT rect;

   GetClientRect(&rect);

   pMap = new nxMap(0, 0, m_pObject->szName, _T("Dependency map"));

   list.AddObject(m_pObject->dwId);
   AddParentsToMap(m_pObject, list);
   
   pSubmap = new nxSubmap((DWORD)0);
   pSubmap->DoLayout(list.GetNumObjects(), list.GetObjects(),
                     list.GetNumLinks(), list.GetLinks(), rect.right, rect.bottom,
                     SUBMAP_LAYOUT_REINGOLD_TILFORD);
   pMap->AddSubmap(pSubmap);
   m_wndMap.SetMap(pMap);
}


//
// Add object's parents to map
//

void CObjectDepView::AddParentsToMap(NXC_OBJECT *pChild, nxObjList &list)
{
   DWORD i;
   NXC_OBJECT *pObject;

   for(i = 0; i < pChild->dwNumParents; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pChild->pdwParentList[i]);
      if (pObject != NULL)
      {
         if (pObject->iClass != OBJECT_TEMPLATE)
         {
            list.AddObject(pChild->pdwParentList[i]);
            list.LinkObjects(pChild->dwId, pChild->pdwParentList[i]);
            AddParentsToMap(pObject, list);
         }
      }
   }
}
