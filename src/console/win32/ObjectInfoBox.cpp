// ObjectInfoBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectInfoBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constants
//

#define X_MARGIN     4
#define Y_MARGIN     4


/////////////////////////////////////////////////////////////////////////////
// CObjectInfoBox

CObjectInfoBox::CObjectInfoBox()
{
   m_pCurrObject = NULL;
   memset(m_hIconList, 0, sizeof(HICON) * 8);
}

CObjectInfoBox::~CObjectInfoBox()
{
}


BEGIN_MESSAGE_MAP(CObjectInfoBox, CToolBox)
	//{{AFX_MSG_MAP(CObjectInfoBox)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Set current object
//

void CObjectInfoBox::SetCurrentObject(NXC_OBJECT *pObject)
{
   m_pCurrObject = pObject;
   Invalidate();
   UpdateWindow();
}


//
// WM_PAINT message handler
//

void CObjectInfoBox::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   if (m_pCurrObject != NULL)
   {
      RECT rect;
      int cy, step, w;
      char szBuffer[64], szTemp[32];
      DWORD dwImageIndex;

      // Draw icon and objet's name right to it
      dc.FillSolidRect(X_MARGIN - 1, Y_MARGIN - 1, 34, 34, g_statusColorTable[m_pCurrObject->iStatus]);
      //dc.DrawIcon(X_MARGIN, Y_MARGIN, m_hIconList[m_pCurrObject->iClass]);
      dwImageIndex = GetObjectImageIndex(m_pCurrObject);
      g_pObjectNormalImageList->Draw(&dc, dwImageIndex, CPoint(X_MARGIN, Y_MARGIN), ILD_TRANSPARENT);
      dc.SelectObject(m_fontNormal);
      dc.SetBkColor(RGB(255, 255, 255));
      GetClientRect(&rect);
      rect.left = X_MARGIN + 37;
      rect.top = Y_MARGIN;
      rect.bottom = Y_MARGIN + 40;
      rect.right -= X_MARGIN;
      dc.DrawText(m_pCurrObject->szName, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
      
      // Current vertical position and increment
      cy = 46;
      step = dc.GetTextExtent("X", 1).cy + 2;

      // Display object's status
      dc.TextOut(X_MARGIN, cy, "Status:", 7);
      dc.SetTextColor(g_statusColorTable[m_pCurrObject->iStatus]);
      w = dc.GetTextExtent("Status: ", 8).cx;
      dc.SelectObject(m_fontBold);
      dc.TextOut(X_MARGIN + w, cy, 
                 g_szStatusText[m_pCurrObject->iStatus], 
                 strlen(g_szStatusText[m_pCurrObject->iStatus]));
      dc.SetTextColor(RGB(0, 0, 0));
      dc.SelectObject(m_fontNormal);
      cy += step;

      // Object's ID and class
      sprintf(szBuffer, "Identifier: %ld", m_pCurrObject->dwId);
      dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
      cy += step;
      sprintf(szBuffer, "Class: %s", g_szObjectClass[m_pCurrObject->iClass]);
      dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
      cy += step;

      // Object's primary IP address
      if (m_pCurrObject->dwIpAddr != 0)
      {
         sprintf(szBuffer, "IP Address: %s", IpToStr(m_pCurrObject->dwIpAddr, szTemp));
         dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
         cy += step;
      }

      // Class-specific information
      switch(m_pCurrObject->iClass)
      {
         case OBJECT_SUBNET:
            sprintf(szBuffer, "IP NetMask: %s", IpToStr(m_pCurrObject->subnet.dwIpNetMask, szTemp));
            dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
            cy += step;
            break;
         case OBJECT_NODE:
            if (m_pCurrObject->node.dwFlags & NF_IS_SNMP)
            {
               dc.TextOut(X_MARGIN, cy, "SNMP supported", 14);
               cy += step;

               sprintf(szBuffer, "OID: %s", m_pCurrObject->node.szObjectId);
               dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
               cy += step;
            }
            break;
         case OBJECT_INTERFACE:
            if (m_pCurrObject->dwIpAddr != 0)
            {
               sprintf(szBuffer, "IP NetMask: %s", IpToStr(m_pCurrObject->iface.dwIpNetMask, szTemp));
               dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
               cy += step;
            }

            sprintf(szBuffer, "Index: %d", m_pCurrObject->iface.dwIfIndex);
            dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
            cy += step;

            sprintf(szBuffer, "Type: %s", m_pCurrObject->iface.dwIfType > MAX_INTERFACE_TYPE ?
                                 "Unknown" : g_szInterfaceTypes[m_pCurrObject->iface.dwIfType]);
            dc.TextOut(X_MARGIN, cy, szBuffer, strlen(szBuffer));
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
      dc.TextOut(X_MARGIN, Y_MARGIN, "No object selected", 18);
   }
}


//
// WM_DESTROY message handler
//

int CObjectInfoBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CToolBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create fonts
   m_fontNormal.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");
   m_fontBold.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");
	
   // Load icons for different object classes
   m_hIconList[1] = theApp.LoadIcon(IDI_SUBNET);      // Subnet
   m_hIconList[2] = theApp.LoadIcon(IDI_NODE);        // Node
   m_hIconList[3] = theApp.LoadIcon(IDI_INTERFACE);   // Interface
   m_hIconList[4] = theApp.LoadIcon(IDI_NETMAP);      // Network
   m_hIconList[5] = NULL;                             // Container
   m_hIconList[6] = NULL;                             // Zone
   m_hIconList[7] = m_hIconList[4];                   // Service root

	return 0;
}


//
// WM_DESTROY message handler
//

void CObjectInfoBox::OnDestroy() 
{
	CToolBox::OnDestroy();
	
   m_fontNormal.DeleteObject();
   m_fontBold.DeleteObject();
}
