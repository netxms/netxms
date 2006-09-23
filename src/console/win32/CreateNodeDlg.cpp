// CreateNodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateNodeDlg.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateNodeDlg dialog


CCreateNodeDlg::CCreateNodeDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateNodeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateNodeDlg)
	m_bDisableAgent = FALSE;
	m_bDisableICMP = FALSE;
	m_bDisableSNMP = FALSE;
	m_bCreateUnmanaged = FALSE;
	//}}AFX_DATA_INIT

   m_dwProxyNode = NULL;
}


void CCreateNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateNodeDlg)
	DDX_Control(pDX, IDC_EDIT_NAME, m_wndObjectName);
	DDX_Control(pDX, IDC_IP_ADDR, m_wndIPAddr);
	DDX_Check(pDX, IDC_CHECK_AGENT, m_bDisableAgent);
	DDX_Check(pDX, IDC_CHECK_ICMP, m_bDisableICMP);
	DDX_Check(pDX, IDC_CHECK_SNMP, m_bDisableSNMP);
	DDX_Check(pDX, IDC_CHECK_UNMANAGED, m_bCreateUnmanaged);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateNodeDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateNodeDlg)
	ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnButtonResolve)
	ON_BN_CLICKED(IDC_SELECT_PROXY, OnSelectProxy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateNodeDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCreateNodeDlg::OnInitDialog() 
{
	CCreateObjectDlg::OnInitDialog();
	
   // Proxy node
   if (m_dwProxyNode != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwProxyNode);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_PROXY, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_PROXY, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_PROXY, _T("<none>"));
   }
	
	return TRUE;
}


//
// "OK" button handler
//

void CCreateNodeDlg::OnOK() 
{
   int iNumBytes;

   iNumBytes = m_wndIPAddr.GetAddress(m_dwIpAddr);
   if ((iNumBytes != 0) && (iNumBytes != 4))
   {
      MessageBox(_T("Invalid IP address"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
   else
   {
      if (iNumBytes == 0)
         m_dwIpAddr = 0;
      if ((m_pParentObject == NULL) && (m_dwIpAddr == 0))
         MessageBox(_T("Node without IP address and parent container object cannot be created"),
                    _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      else
	      CCreateObjectDlg::OnOK();
   }
}


//
// "Resolve" button handler
//

void CCreateNodeDlg::OnButtonResolve() 
{
   char szHostName[256];
   struct hostent *hs;
   DWORD dwAddr = INADDR_NONE;

#ifdef UNICODE
   WCHAR wszTemp[256];

   m_wndObjectName.GetWindowText(wszTemp, 256);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wszTemp, -1,
                       szHostName, 256, NULL, NULL);
#else
   m_wndObjectName.GetWindowText(szHostName, 256);
#endif
   hs = gethostbyname(szHostName);
   if (hs != NULL)
   {
      memcpy(&dwAddr, hs->h_addr, sizeof(DWORD));
   }
   else
   {
      dwAddr = inet_addr(szHostName);
   }

   if (dwAddr != INADDR_NONE)
   {
      m_wndIPAddr.SetAddress(ntohl(dwAddr));
   }
   else
   {
      MessageBox(_T("Unable to resolve host name!"), _T("Error"), MB_OK | MB_ICONSTOP);
   }
}


//
// Handler for proxy node selection button
//

void CCreateNodeDlg::OnSelectProxy() 
{
   CObjectSelDlg dlg;

   dlg.m_dwAllowedClasses = SCL_NODE;
   dlg.m_bSingleSelection = TRUE;
   dlg.m_bAllowEmptySelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      if (dlg.m_dwNumObjects != 0)
      {
         NXC_OBJECT *pNode;

         m_dwProxyNode = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwProxyNode);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_PROXY, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_PROXY, _T("<invalid>"));
         }
      }
      else
      {
         m_dwProxyNode = 0;
         SetDlgItemText(IDC_EDIT_PROXY, _T("<none>"));
      }
   }
}
