// NetSrvPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NetSrvPropsGeneral.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetSrvPropsGeneral property page

IMPLEMENT_DYNCREATE(CNetSrvPropsGeneral, CPropertyPage)

CNetSrvPropsGeneral::CNetSrvPropsGeneral() : CPropertyPage(CNetSrvPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CNetSrvPropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	m_iPort = 0;
	m_strRequest = _T("");
	m_strResponse = _T("");
	m_iProto = 0;
	//}}AFX_DATA_INIT
}

CNetSrvPropsGeneral::~CNetSrvPropsGeneral()
{
}

void CNetSrvPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetSrvPropsGeneral)
	DDX_Control(pDX, IDC_COMBO_TYPE, m_wndTypeList);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_PORT, m_iPort);
	DDV_MinMaxLong(pDX, m_iPort, 0, 65535);
	DDX_Text(pDX, IDC_EDIT_REQUEST, m_strRequest);
	DDX_Text(pDX, IDC_EDIT_RESPONSE, m_strResponse);
	DDX_Text(pDX, IDC_EDIT_PROTO, m_iProto);
	DDV_MinMaxLong(pDX, m_iProto, 0, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNetSrvPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CNetSrvPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPE, OnSelchangeComboType)
	ON_EN_CHANGE(IDC_EDIT_REQUEST, OnChangeEditRequest)
	ON_EN_CHANGE(IDC_EDIT_RESPONSE, OnChangeEditResponse)
	ON_BN_CLICKED(IDC_RADIO_TCP, OnRadioTcp)
	ON_BN_CLICKED(IDC_RADIO_UDP, OnRadioUdp)
	ON_BN_CLICKED(IDC_RADIO_ICMP, OnRadioIcmp)
	ON_BN_CLICKED(IDC_RADIO_OTHER, OnRadioOther)
	ON_BN_CLICKED(IDC_SELECT_IP, OnSelectIp)
	ON_BN_CLICKED(IDC_SELECT_POLLER, OnSelectPoller)
	ON_EN_CHANGE(IDC_EDIT_PORT, OnChangeEditPort)
	ON_EN_CHANGE(IDC_EDIT_PROTO, OnChangeEditProto)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetSrvPropsGeneral message handlers


//
// WM_INITDIALOG message handler
//

BOOL CNetSrvPropsGeneral::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

   // Add service types to combo box
   for(i = 0; g_szServiceType[i] != NULL; i++)
      m_wndTypeList.AddString(g_szServiceType[i]);
   m_wndTypeList.SelectString(-1, g_szServiceType[m_iServiceType]);

   // Poller node
   if (m_dwPollerNode != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_POLLER, _T("<default>"));
   }

   // Listen IP address
   if (m_dwIpAddr != 0)
   {
      TCHAR szBuffer[16];

      IpToStr(m_dwIpAddr, szBuffer);
      SetDlgItemText(IDC_EDIT_IPADDR, szBuffer);
   }
   else
   {
      SetDlgItemText(IDC_EDIT_IPADDR, _T("<any>"));
   }

   SetProtocolCtrls();
	
	return TRUE;
}


//
// Handler for OK button
//

void CNetSrvPropsGeneral::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
   m_pUpdate->pszRequest = (TCHAR *)((LPCTSTR)m_strRequest);
   m_pUpdate->pszResponse = (TCHAR *)((LPCTSTR)m_strResponse);
   m_pUpdate->iServiceType = m_iServiceType;
   m_pUpdate->wPort = (WORD)m_iPort;
   m_pUpdate->wProto = (WORD)m_iProto;
   m_pUpdate->dwPollerNode = m_dwPollerNode;
   m_pUpdate->dwIpAddr = m_dwIpAddr;
}


//
// Handlers for changed controls
//

void CNetSrvPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}

void CNetSrvPropsGeneral::OnChangeEditRequest() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_CHECK_REQUEST;
   SetModified();
}

void CNetSrvPropsGeneral::OnChangeEditResponse() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_CHECK_RESPONSE;
   SetModified();
}

void CNetSrvPropsGeneral::OnChangeEditPort() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PORT;
   SetModified();
}

void CNetSrvPropsGeneral::OnChangeEditProto() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PROTO;
   SetModified();
}

void CNetSrvPropsGeneral::OnSelchangeComboType() 
{
   TCHAR szBuffer[256];

   // Determine service type
   m_wndTypeList.GetWindowText(szBuffer, 256);
   for(m_iServiceType = 0; g_szServiceType[m_iServiceType] != NULL; m_iServiceType++)
      if (!_tcscmp(szBuffer, g_szServiceType[m_iServiceType]))
         break;
   SetProtocolCtrls();

   m_pUpdate->dwFlags |= OBJ_UPDATE_SERVICE_TYPE;
   SetModified();
}

void CNetSrvPropsGeneral::OnRadioTcp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("6"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PROTO;
   SetModified();
}

void CNetSrvPropsGeneral::OnRadioUdp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("17"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PROTO;
   SetModified();
}

void CNetSrvPropsGeneral::OnRadioIcmp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("1"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PROTO;
   SetModified();
}

void CNetSrvPropsGeneral::OnRadioOther() 
{
   EnableDlgItem(this, IDC_EDIT_PROTO, TRUE);
   m_pUpdate->dwFlags |= OBJ_UPDATE_IP_PROTO;
   SetModified();
}


//
// Set states of IP protocol selection controls
//

void CNetSrvPropsGeneral::SetProtocolCtrls()
{
   BOOL bEnable;

   if (m_iServiceType == NETSRV_CUSTOM)
   {
      switch(m_iProto)
      {
         case IPPROTO_TCP:
            SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_CHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_UNCHECKED, 0);
            break;
         case IPPROTO_UDP:
            SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_CHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_UNCHECKED, 0);
            break;
         case IPPROTO_ICMP:
            SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_CHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_UNCHECKED, 0);
            break;
         default:
            SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_CHECKED, 0);
            break;
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_PROTO, _T("6"));
      SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_CHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_UNCHECKED, 0);
   }

   bEnable = (m_iServiceType == NETSRV_CUSTOM) ? TRUE : FALSE;
   EnableDlgItem(this, IDC_RADIO_TCP, bEnable);
   EnableDlgItem(this, IDC_RADIO_UDP, bEnable);
   EnableDlgItem(this, IDC_RADIO_ICMP, bEnable);
   EnableDlgItem(this, IDC_RADIO_OTHER, bEnable);
   EnableDlgItem(this, IDC_EDIT_PROTO, 
                 SendDlgItemMessage(IDC_RADIO_OTHER, BM_GETCHECK) == BST_CHECKED);
}


//
// Select IP address for binding
//

void CNetSrvPropsGeneral::OnSelectIp() 
{
   CObjectSelDlg dlg;
   NXC_OBJECT *pObject;
   DWORD i;

   // Find host node
   for(i = 0; i < m_pObject->dwNumParents; i++)
   {
      pObject = NXCFindObjectById(g_hSession, m_pObject->pdwParentList[i]);
      if (pObject != NULL)
      {
         if (pObject->iClass == OBJECT_NODE)
         {
            dlg.m_dwParentObject = m_pObject->pdwParentList[i];
            break;
         }
      }
   }

   if (dlg.m_dwParentObject != 0)
   {
      dlg.m_bSelectAddress = TRUE;
      dlg.m_bSingleSelection = TRUE;
      dlg.m_bAllowEmptySelection = TRUE;
      dlg.m_bShowLoopback = TRUE;
      if (dlg.DoModal() == IDOK)
      {
         if (dlg.m_dwNumObjects != 0)
         {
            TCHAR szBuffer[16];

            if (dlg.m_pdwObjectList[0] != 0)
            {
               pObject = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
               if (pObject != NULL)
                  m_dwIpAddr = pObject->dwIpAddr;
            }
            else
            {
               // Loopback
               m_dwIpAddr = 0x7F000001;
            }
            SetDlgItemText(IDC_EDIT_IPADDR, IpToStr(m_dwIpAddr, szBuffer));
         }
         else
         {
            m_dwIpAddr = 0;
            SetDlgItemText(IDC_EDIT_IPADDR, _T("<any>"));
         }
         m_pUpdate->dwFlags |= OBJ_UPDATE_IP_ADDR;
         SetModified();
      }
   }
   else
   {
      MessageBox(_T("Unable to determine host node for this service!"),
                 _T("Internal Error"), MB_ICONSTOP | MB_OK);
   }
}


//
// Select poller node
//

void CNetSrvPropsGeneral::OnSelectPoller() 
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

         m_dwPollerNode = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
         }
      }
      else
      {
         m_dwPollerNode = 0;
         SetDlgItemText(IDC_EDIT_POLLER, _T("<default>"));
      }
      m_pUpdate->dwFlags |= OBJ_UPDATE_POLLER_NODE;
      SetModified();
   }
}
