// CondPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondPropsGeneral.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"
#include "EventSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCondPropsGeneral property page

IMPLEMENT_DYNCREATE(CCondPropsGeneral, CPropertyPage)

CCondPropsGeneral::CCondPropsGeneral() : CPropertyPage(CCondPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CCondPropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	//}}AFX_DATA_INIT
}

CCondPropsGeneral::~CCondPropsGeneral()
{
}

void CCondPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCondPropsGeneral)
	DDX_Control(pDX, IDC_COMBO_INACTIVE, m_wndComboInactive);
	DDX_Control(pDX, IDC_COMBO_ACTIVE, m_wndComboActive);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCondPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CCondPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_CBN_SELCHANGE(IDC_COMBO_ACTIVE, OnSelchangeComboActive)
	ON_CBN_SELCHANGE(IDC_COMBO_INACTIVE, OnSelchangeComboInactive)
	ON_BN_CLICKED(IDC_BROWSE_OBJECT, OnBrowseObject)
	ON_BN_CLICKED(IDC_BROWSE_ACTIVATION_EVENT, OnBrowseActivationEvent)
	ON_BN_CLICKED(IDC_BROWSE_DEACTIVATION_EVENT, OnBrowseDeactivationEvent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCondPropsGeneral message handlers

BOOL CCondPropsGeneral::OnInitDialog() 
{
   int i;
   NXC_OBJECT *pObject;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   // Initialize status selectors
   for(i = STATUS_NORMAL; i <= STATUS_CRITICAL; i++)
   {
      m_wndComboActive.AddString(g_szStatusTextSmall[i]);
      m_wndComboInactive.AddString(g_szStatusTextSmall[i]);
   }
   m_wndComboActive.SelectString(-1, g_szStatusTextSmall[m_pObject->cond.wActiveStatus]);
   m_wndComboInactive.SelectString(-1, g_szStatusTextSmall[m_pObject->cond.wInactiveStatus]);

   // Source object name
   if (m_pObject->cond.dwSourceObject != 0)
   {
      pObject = NXCFindObjectById(g_hSession, m_pObject->cond.dwSourceObject);
      SetDlgItemText(IDC_EDIT_OBJECT, pObject->szName);
   }
   else
   {
      SetDlgItemText(IDC_EDIT_OBJECT, _T("<server>"));
   }

   // Events
   SetDlgItemText(IDC_EDIT_ACTIVATION_EVENT, NXCGetEventName(g_hSession, m_pObject->cond.dwActivationEvent));
   SetDlgItemText(IDC_EDIT_DEACTIVATION_EVENT, NXCGetEventName(g_hSession, m_pObject->cond.dwDeactivationEvent));

	return TRUE;
}


//
// Handler for PSN_OK message
//

void CCondPropsGeneral::OnOK() 
{
   TCHAR szBuffer[256];
   int i;

	CPropertyPage::OnOK();

   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);

   // Active status
   m_wndComboActive.GetWindowText(szBuffer, 256);
   for(i = STATUS_NORMAL; i <= STATUS_CRITICAL; i++)
   {
      if (!_tcscmp(szBuffer, g_szStatusTextSmall[i]))
      {
         m_pUpdate->nActiveStatus = i;
         break;
      }
   }

   // Inactive status
   m_wndComboInactive.GetWindowText(szBuffer, 256);
   for(i = STATUS_NORMAL; i <= STATUS_CRITICAL; i++)
   {
      if (!_tcscmp(szBuffer, g_szStatusTextSmall[i]))
      {
         m_pUpdate->nInactiveStatus = i;
         break;
      }
   }
}


//
// Handlers for changed controls
//

void CCondPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}

void CCondPropsGeneral::OnSelchangeComboActive() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_ACTIVE_STATUS;
   SetModified();
}

void CCondPropsGeneral::OnSelchangeComboInactive() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_INACTIVE_STATUS;
   SetModified();
}


//
// Handler for "Select source object" button
//

void CCondPropsGeneral::OnBrowseObject() 
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

         m_pUpdate->dwSourceObject = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_pUpdate->dwSourceObject);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_OBJECT, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_OBJECT, _T("<invalid>"));
         }
      }
      else
      {
         m_pUpdate->dwSourceObject = 0;
         SetDlgItemText(IDC_EDIT_OBJECT, _T("<server>"));
      }
      m_pUpdate->dwFlags |= OBJ_UPDATE_SOURCE_OBJECT;
      SetModified();
   }
}


//
// Handler for "Select activation event" button
//

void CCondPropsGeneral::OnBrowseActivationEvent() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_pUpdate->dwActivationEvent = dlg.m_pdwEventList[0];
      SetDlgItemText(IDC_EDIT_ACTIVATION_EVENT, NXCGetEventName(g_hSession, dlg.m_pdwEventList[0]));
      m_pUpdate->dwFlags |= OBJ_UPDATE_ACT_EVENT;
   }
}


//
// Handler for "Select deactivation event" button
//

void CCondPropsGeneral::OnBrowseDeactivationEvent() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_pUpdate->dwDeactivationEvent = dlg.m_pdwEventList[0];
      SetDlgItemText(IDC_EDIT_DEACTIVATION_EVENT, NXCGetEventName(g_hSession, dlg.m_pdwEventList[0]));
      m_pUpdate->dwFlags |= OBJ_UPDATE_DEACT_EVENT;
   }
}
