// ObjectPropsTrustedNodes.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsTrustedNodes.h"
#include "ObjectSelDlg.h"
#include "ObjectPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsTrustedNodes property page

IMPLEMENT_DYNCREATE(CObjectPropsTrustedNodes, CPropertyPage)

CObjectPropsTrustedNodes::CObjectPropsTrustedNodes() : CPropertyPage(CObjectPropsTrustedNodes::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsTrustedNodes)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CObjectPropsTrustedNodes::~CObjectPropsTrustedNodes()
{
}

void CObjectPropsTrustedNodes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsTrustedNodes)
	DDX_Control(pDX, IDC_LIST_NODES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsTrustedNodes, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsTrustedNodes)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsTrustedNodes message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsTrustedNodes::OnInitDialog() 
{
	DWORD i;
	RECT rect;
	NXC_OBJECT *pObject;
	TCHAR buffer[32];
	int item;

	CPropertyPage::OnInitDialog();
	
   // Setup list controls
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, rect.right - 70 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   m_imageList.Create(g_pObjectSmallImageList);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Fill node list
   for(i = 0; i < m_dwNumNodes; i++)
   {
      pObject = NXCFindObjectById(g_hSession, m_pdwNodeList[i]);
      if (pObject != NULL)
      {
			_sntprintf_s(buffer, 32, _TRUNCATE, _T("%d"), pObject->dwId);
         item = m_wndListCtrl.InsertItem(0x7FFFFFFF, buffer, pObject->iClass);
         if (item != -1)
         {
				m_wndListCtrl.SetItemText(item, 1, pObject->szName);
            m_wndListCtrl.SetItemData(item, pObject->dwId);
         }
      }
   }

	m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

	return TRUE;
}


//
// OK button handler
//

void CObjectPropsTrustedNodes::OnOK() 
{
	DWORD i;

	CPropertyPage::OnOK();
	if (m_pUpdate->qwFlags & OBJ_UPDATE_TRUSTED_NODES)
	{
		m_pUpdate->dwNumTrustedNodes = m_wndListCtrl.GetItemCount();
		m_pUpdate->pdwTrustedNodes = (UINT32 *)malloc(sizeof(UINT32) * m_pUpdate->dwNumTrustedNodes);
		for(i = 0; i < m_pUpdate->dwNumTrustedNodes; i++)
		{
			m_pUpdate->pdwTrustedNodes[i] = (UINT32)m_wndListCtrl.GetItemData(i);
		}
	}
}


//
// Delete nodes
//

void CObjectPropsTrustedNodes::OnButtonDelete() 
{
	int item;
	BOOL changed = FALSE;

	while((item = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
	{
		m_wndListCtrl.DeleteItem(item);
		changed = TRUE;
	}

	if (changed)
	{
		m_pUpdate->qwFlags |= OBJ_UPDATE_TRUSTED_NODES;
		SetModified();
	}
}


//
// Add nodes
//

void CObjectPropsTrustedNodes::OnButtonAdd() 
{
	CObjectSelDlg dlg;
	LVFINDINFO lvfi;
	DWORD i;
	TCHAR buffer[32];
	int item;
	NXC_OBJECT *object;
	BOOL changed = FALSE;

	dlg.m_dwAllowedClasses = SCL_NODE;
	if (dlg.DoModal() == IDOK)
	{
		lvfi.flags = LVFI_PARAM;
		for(i = 0; i < dlg.m_dwNumObjects; i++)
		{
			lvfi.lParam = dlg.m_pdwObjectList[i];
			item = m_wndListCtrl.FindItem(&lvfi);
			if (item == -1)
			{
				// No such object in list
				object = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[i]);
				if (object != NULL)
				{
					_sntprintf_s(buffer, 32, _TRUNCATE, _T("%d"), object->dwId);
					item = m_wndListCtrl.InsertItem(0x7FFFFFFF, buffer, object->iClass);
					if (item != -1)
					{
						m_wndListCtrl.SetItemText(item, 1, object->szName);
						m_wndListCtrl.SetItemData(item, object->dwId);
						changed = TRUE;
					}
				}
			}
		}
	}

	if (changed)
	{
		m_pUpdate->qwFlags |= OBJ_UPDATE_TRUSTED_NODES;
		SetModified();
	}
}
