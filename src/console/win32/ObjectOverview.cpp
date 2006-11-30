// ObjectOverview.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectOverview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define X_MARGIN     4
#define Y_MARGIN     4


/////////////////////////////////////////////////////////////////////////////
// CObjectOverview

CObjectOverview::CObjectOverview()
{
   m_pObject = NULL;
}

CObjectOverview::~CObjectOverview()
{
}


BEGIN_MESSAGE_MAP(CObjectOverview, CWnd)
	//{{AFX_MSG_MAP(CObjectOverview)
	ON_WM_CREATE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectOverview message handlers

BOOL CObjectOverview::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectOverview::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create fonts
   m_fontNormal.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_fontBold.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
	
	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

void CObjectOverview::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
   Invalidate();
}


//
// WM_PAINT message handler
//

void CObjectOverview::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   if (m_pObject != NULL)
   {
      RECT rect;
      int cy, step, w;
      TCHAR szBuffer[64], szTemp[32];
      DWORD dwImageIndex;

      // Draw icon and objet's name right to it
      dc.FillSolidRect(X_MARGIN - 1, Y_MARGIN - 1, 34, 34, g_statusColorTable[m_pObject->iStatus]);
      dwImageIndex = GetObjectImageIndex(m_pObject);
      g_pObjectNormalImageList->Draw(&dc, dwImageIndex, CPoint(X_MARGIN, Y_MARGIN), ILD_TRANSPARENT);
      dc.SelectObject(m_fontNormal);
      dc.SetBkColor(RGB(255, 255, 255));
      GetClientRect(&rect);
      rect.left = X_MARGIN + 37;
      rect.top = Y_MARGIN;
      rect.bottom = Y_MARGIN + 40;
      rect.right -= X_MARGIN;
      dc.DrawText(m_pObject->szName, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
      
      // Current vertical position and increment
      cy = 46;
      step = dc.GetTextExtent(_T("X"), 1).cy + 2;

      // Display object's status
      dc.TextOut(X_MARGIN, cy, _T("Status:"), 7);
      dc.SetTextColor(g_statusColorTable[m_pObject->iStatus]);
      w = dc.GetTextExtent(_T("Status: "), 8).cx;
      dc.SelectObject(m_fontBold);
      dc.TextOut(X_MARGIN + w, cy, 
                 g_szStatusText[m_pObject->iStatus], 
                 _tcslen(g_szStatusText[m_pObject->iStatus]));
      dc.SetTextColor(RGB(0, 0, 0));
      dc.SelectObject(m_fontNormal);
      cy += step;

      // Object's ID and class
      _stprintf(szBuffer, _T("Identifier: %d"), m_pObject->dwId);
      dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
      cy += step;
      _stprintf(szBuffer, _T("Class: %s"), g_szObjectClass[m_pObject->iClass]);
      dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
      cy += step;

      // Object's primary IP address
      if (m_pObject->dwIpAddr != 0)
      {
         _stprintf(szBuffer, _T("IP Address: %s"), IpToStr(m_pObject->dwIpAddr, szTemp));
         dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
         cy += step;
      }

      // Class-specific information
      switch(m_pObject->iClass)
      {
         case OBJECT_SUBNET:
            _stprintf(szBuffer, _T("IP NetMask: %s"), IpToStr(m_pObject->subnet.dwIpNetMask, szTemp));
            dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
            cy += step;
            break;
         case OBJECT_NODE:
            if (m_pObject->node.dwFlags & NF_IS_SNMP)
            {
               dc.TextOut(X_MARGIN, cy, _T("SNMP supported"), 14);
               cy += step;

               _stprintf(szBuffer, _T("OID: %s"), m_pObject->node.szObjectId);
               dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
               cy += step;
            }
            break;
         case OBJECT_INTERFACE:
            if (m_pObject->dwIpAddr != 0)
            {
               _stprintf(szBuffer, _T("IP NetMask: %s"), IpToStr(m_pObject->iface.dwIpNetMask, szTemp));
               dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
               cy += step;
            }

            _stprintf(szBuffer, _T("Index: %d"), m_pObject->iface.dwIfIndex);
            dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
            cy += step;

            _stprintf(szBuffer, _T("Type: %s"), 
                      m_pObject->iface.dwIfType > MAX_INTERFACE_TYPE ?
                          _T("Unknown") : g_szInterfaceTypes[m_pObject->iface.dwIfType]);
            dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
            cy += step;

            _tcscpy(szBuffer, _T("MAC: "));
            BinToStr(m_pObject->iface.bMacAddr, MAC_ADDR_LENGTH, &szBuffer[5]);
            dc.TextOut(X_MARGIN, cy, szBuffer, _tcslen(szBuffer));
            cy += step;
            break;
         default:
            break;
      }
   }
   else
   {
      dc.SelectObject(m_fontNormal);
      dc.SetBkColor(RGB(255, 255, 255));
      dc.TextOut(X_MARGIN, Y_MARGIN, _T("No object selected"), 18);
   }
}
