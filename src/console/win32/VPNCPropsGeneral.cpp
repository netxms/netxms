// VPNCPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "VPNCPropsGeneral.h"
#include "EditSubnetDlg.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVPNCPropsGeneral property page

IMPLEMENT_DYNCREATE(CVPNCPropsGeneral, CPropertyPage)

CVPNCPropsGeneral::CVPNCPropsGeneral() : CPropertyPage(CVPNCPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CVPNCPropsGeneral)
	m_strName = _T("");
	m_dwObjectId = 0;
	//}}AFX_DATA_INIT

   m_pLocalNetList = NULL;
   m_pRemoteNetList = NULL;
}

CVPNCPropsGeneral::~CVPNCPropsGeneral()
{
   safe_free(m_pLocalNetList);
   safe_free(m_pRemoteNetList);
}

void CVPNCPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVPNCPropsGeneral)
	DDX_Control(pDX, IDC_LIST_REMOTE, m_wndListRemote);
	DDX_Control(pDX, IDC_LIST_LOCAL, m_wndListLocal);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVPNCPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CVPNCPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_BN_CLICKED(IDC_SELECT_GATEWAY, OnSelectGateway)
	ON_BN_CLICKED(IDC_ADD_LOCAL_NET, OnAddLocalNet)
	ON_BN_CLICKED(IDC_ADD_REMOTE_NET, OnAddRemoteNet)
	ON_BN_CLICKED(IDC_REMOVE_LOCAL_NET, OnRemoveLocalNet)
	ON_BN_CLICKED(IDC_REMOVE_REMOTE_NET, OnRemoveRemoteNet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVPNCPropsGeneral message handlers


//
// WM_INITDIALOG message handler
//

BOOL CVPNCPropsGeneral::OnInitDialog() 
{
   DWORD i;
   RECT rect;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

   // Peer gateway
   if (m_dwPeerGateway != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwPeerGateway);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_GATEWAY, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_GATEWAY, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_GATEWAY, _T("<none>"));
   }

   // Setup list controls
   m_wndListLocal.GetClientRect(&rect);
   m_wndListLocal.InsertColumn(0, _T("Net"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListLocal.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListRemote.GetClientRect(&rect);
   m_wndListRemote.InsertColumn(0, _T("Net"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListRemote.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   // Network list
   m_dwNumLocalNets = m_pObject->vpnc.dwNumLocalNets;
   m_pLocalNetList = (IP_NETWORK *)nx_memdup(m_pObject->vpnc.pLocalNetList,
                                             sizeof(IP_NETWORK) * m_dwNumLocalNets);
   m_dwNumRemoteNets = m_pObject->vpnc.dwNumRemoteNets;
   m_pRemoteNetList = (IP_NETWORK *)nx_memdup(m_pObject->vpnc.pRemoteNetList,
                                              sizeof(IP_NETWORK) * m_dwNumRemoteNets);

   for(i = 0; i < m_dwNumLocalNets; i++)
      AddSubnetToList(&m_wndListLocal, &m_pLocalNetList[i]);
   for(i = 0; i < m_dwNumRemoteNets; i++)
      AddSubnetToList(&m_wndListRemote, &m_pRemoteNetList[i]);
	
	return TRUE;
}


//
// Handler for OK button
//

void CVPNCPropsGeneral::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
   m_pUpdate->dwPeerGateway = m_dwPeerGateway;
   m_pUpdate->dwNumLocalNets = m_dwNumLocalNets;
   m_pUpdate->dwNumRemoteNets = m_dwNumRemoteNets;
   m_pUpdate->pLocalNetList = m_pLocalNetList;
   m_pUpdate->pRemoteNetList = m_pRemoteNetList;
}


//
// Handle object name's changes
//

void CVPNCPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}


//
// Select peer gateway
//

void CVPNCPropsGeneral::OnSelectGateway() 
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

         m_dwPeerGateway = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwPeerGateway);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_GATEWAY, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_GATEWAY, _T("<invalid>"));
         }
      }
      else
      {
         m_dwPeerGateway = 0;
         SetDlgItemText(IDC_EDIT_GATEWAY, _T("<none>"));
      }
      m_pUpdate->dwFlags |= OBJ_UPDATE_PEER_GATEWAY;
      SetModified();
   }
}


//
// Compare subnets for sorting
//

static int CALLBACK CompareSubnets(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   return COMPARE_NUMBERS(lParam1, lParam2);
}


//
// Add subnet to list
//

void CVPNCPropsGeneral::AddSubnetToList(CListCtrl *pListCtrl, IP_NETWORK *pSubnet)
{
   TCHAR szBuffer[64], szIpAddr[16];
   int iItem;

   _sntprintf(szBuffer, 64, _T("%s/%d"), IpToStr(pSubnet->dwAddr, szIpAddr), BitsInMask(pSubnet->dwMask));
   iItem = pListCtrl->InsertItem(0x7FFFFFFF, szBuffer);
   if (iItem != -1)
   {
      pListCtrl->SetItemData(iItem, pSubnet->dwAddr);
      pListCtrl->SortItems(CompareSubnets, 0);
   }
}


//
// Add new network to list
//

void CVPNCPropsGeneral::AddNetwork(DWORD *pdwNumNets, IP_NETWORK **ppNetList, CListCtrl *pListCtrl)
{
   CEditSubnetDlg dlg;

   if (dlg.DoModal() == IDOK)
   {
      *ppNetList = (IP_NETWORK *)realloc(*ppNetList, sizeof(IP_NETWORK) * (*pdwNumNets + 1));
      memcpy(&(*ppNetList)[*pdwNumNets], &dlg.m_subnet, sizeof(IP_NETWORK));
      (*pdwNumNets)++;

      AddSubnetToList(pListCtrl, &dlg.m_subnet);

      m_pUpdate->dwFlags |= OBJ_UPDATE_NETWORK_LIST;
      SetModified();
   }
}


//
// Add new local network
//

void CVPNCPropsGeneral::OnAddLocalNet() 
{
   AddNetwork(&m_dwNumLocalNets, &m_pLocalNetList, &m_wndListLocal);
}


//
// Add new remote network
//

void CVPNCPropsGeneral::OnAddRemoteNet() 
{
   AddNetwork(&m_dwNumRemoteNets, &m_pRemoteNetList, &m_wndListRemote);
}


//
// Remove local network
//

void CVPNCPropsGeneral::OnRemoveLocalNet() 
{
   RemoveNetworks(&m_dwNumLocalNets, &m_pLocalNetList, &m_wndListLocal);
}


//
// Remove remote network
//

void CVPNCPropsGeneral::OnRemoveRemoteNet() 
{
   RemoveNetworks(&m_dwNumRemoteNets, &m_pRemoteNetList, &m_wndListRemote);
}


//
// Remove network(s) from list
//

void CVPNCPropsGeneral::RemoveNetworks(DWORD *pdwNumNets, IP_NETWORK **ppNetList, CListCtrl *pListCtrl)
{
   int iItem, nDeleted = 0;
   DWORD i, dwIpAddr;

   while(1)
   {
      iItem = pListCtrl->GetNextItem(-1, LVNI_SELECTED);
      if (iItem == -1)
         break;
      dwIpAddr = pListCtrl->GetItemData(iItem);
      for(i = 0; i < *pdwNumNets; i++)
         if ((*ppNetList)[i].dwAddr == dwIpAddr)
            break;
      if (i < *pdwNumNets)
      {
         (*pdwNumNets)--;
         memmove(&(*ppNetList)[i], &(*ppNetList)[i + 1], sizeof(IP_NETWORK) * (*pdwNumNets - i));
         nDeleted++;
      }
      pListCtrl->DeleteItem(iItem);
   }

   if (nDeleted > 0)
   {
      m_pUpdate->dwFlags |= OBJ_UPDATE_NETWORK_LIST;
      SetModified();
   }
}
