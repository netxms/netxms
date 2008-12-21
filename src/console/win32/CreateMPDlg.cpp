// CreateMPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateMPDlg.h"
#include "EventSelDlg.h"
#include "ObjectSelDlg.h"
#include "TrapSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define EVENT_IMAGE_BASE      4
#define TEMPLATE_IMAGE        3


/////////////////////////////////////////////////////////////////////////////
// CCreateMPDlg dialog


CCreateMPDlg::CCreateMPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateMPDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateMPDlg)
	m_strDescription = _T("");
	m_strFile = _T("");
	//}}AFX_DATA_INIT

   m_dwNumEvents = 0;
   m_dwNumTemplates = 0;
   m_dwNumTraps = 0;
   m_pdwEventList = NULL;
   m_pdwTemplateList = NULL;
   m_pdwTrapList = NULL;
}

CCreateMPDlg::~CCreateMPDlg()
{
   safe_free(m_pdwEventList);
   safe_free(m_pdwTemplateList);
   safe_free(m_pdwTrapList);
   if (m_pTrapCfg != NULL)
      NXCDestroyTrapList(m_dwTrapCfgSize, m_pTrapCfg);
}

void CCreateMPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateMPDlg)
	DDX_Control(pDX, IDC_TREE_CONTENT, m_wndTreeCtrl);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFile);
	DDV_MaxChars(pDX, m_strFile, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateMPDlg, CDialog)
	//{{AFX_MSG_MAP(CCreateMPDlg)
	ON_BN_CLICKED(IDC_BUTTON_ADD_EVENT, OnButtonAddEvent)
	ON_BN_CLICKED(IDC_BUTTON_ADD_TEMPLATE, OnButtonAddTemplate)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD_TRAP, OnButtonAddTrap)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateMPDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCreateMPDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   DoRequestArg3(NXCGetTrapCfgRO, g_hSession, &m_dwTrapCfgSize, &m_pTrapCfg,
                 _T("Loading SNMP trap mappings..."));

   // Create image list for tree control
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 4);
   m_imageList.Add(theApp.LoadIcon(IDI_LOG));
   m_imageList.Add(theApp.LoadIcon(IDI_TEMPLATE_ROOT));
   m_imageList.Add(theApp.LoadIcon(IDI_TRAP));
   m_imageList.Add(theApp.LoadIcon(IDI_DCT));
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
   m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

   // Create root items
   m_hEventRoot = m_wndTreeCtrl.InsertItem(_T("Events"), 0, 0);
   m_hTemplateRoot = m_wndTreeCtrl.InsertItem(_T("Templates"), 1, 1);
   m_hTrapRoot = m_wndTreeCtrl.InsertItem(_T("SNMP Traps"), 2, 2);
	
	return TRUE;
}


//
// Handler for "Add event..." button
//

void CCreateMPDlg::OnButtonAddEvent() 
{
   CEventSelDlg dlg;
   DWORD i;

   if (dlg.DoModal() == IDOK)
   {
      for(i = 0; i < dlg.m_dwNumEvents; i++)
         AddEvent(dlg.m_pdwEventList[i]);
      m_wndTreeCtrl.SortChildren(m_hEventRoot);
      m_wndTreeCtrl.Expand(m_hEventRoot, TVE_EXPAND);
   }
}


//
// Add event to tree
//

void CCreateMPDlg::AddEvent(DWORD dwId)
{
   int nImage; 
   HTREEITEM hItem;

   if (FindTreeCtrlItemEx(m_wndTreeCtrl, m_hEventRoot, dwId) != NULL)
      return;  // Event already exist in the tree

   nImage = NXCGetEventSeverity(g_hSession, dwId) + EVENT_IMAGE_BASE;
   hItem = m_wndTreeCtrl.InsertItem(NXCGetEventName(g_hSession, dwId),
                                    nImage, nImage, m_hEventRoot);
   m_wndTreeCtrl.SetItemData(hItem, dwId);
}


//
// Handler for "Add template..." button
//

void CCreateMPDlg::OnButtonAddTemplate() 
{
   CObjectSelDlg dlg;
   DWORD i;

   dlg.m_dwAllowedClasses = SCL_TEMPLATE;
   if (dlg.DoModal() == IDOK)
   {
      for(i = 0; i < dlg.m_dwNumObjects; i++)
         AddTemplate(dlg.m_pdwObjectList[i]);
      m_wndTreeCtrl.SortChildren(m_hTemplateRoot);
      m_wndTreeCtrl.Expand(m_hTemplateRoot, TVE_EXPAND);
      m_wndTreeCtrl.SortChildren(m_hEventRoot);
   }
}


//
// Add template to tree
//

void CCreateMPDlg::AddTemplate(DWORD dwId)
{
   HTREEITEM hItem;
   NXC_OBJECT *pObject;
   DWORD i, dwResult, dwCount, *pdwList;

   if (FindTreeCtrlItemEx(m_wndTreeCtrl, m_hTemplateRoot, dwId) != NULL)
      return;  // Template already exist in the tree

   pObject = NXCFindObjectById(g_hSession, dwId);
   hItem = m_wndTreeCtrl.InsertItem(pObject->szName, TEMPLATE_IMAGE,
                                    TEMPLATE_IMAGE, m_hTemplateRoot);
   m_wndTreeCtrl.SetItemData(hItem, dwId);

   // Add all related events
   dwResult = DoRequestArg4(NXCGetDCIEventsList, g_hSession,
                            (void *)dwId, &pdwList, &dwCount,
                            _T("Loading node's data collection information..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwCount; i++)
      {
         AddEvent(pdwList[i]);
      }
      safe_free(pdwList);
   }
}


//
// Add trap to tree
//

void CCreateMPDlg::AddTrap(DWORD dwId, DWORD dwEvent, TCHAR *pszName)
{
   HTREEITEM hItem;
	int nImage;

   if (FindTreeCtrlItemEx(m_wndTreeCtrl, m_hTrapRoot, dwId) != NULL)
      return;  // Template already exist in the tree

   nImage = NXCGetEventSeverity(g_hSession, dwEvent) + EVENT_IMAGE_BASE;
	hItem = m_wndTreeCtrl.InsertItem(pszName, nImage, nImage, m_hTrapRoot);
	m_wndTreeCtrl.SetItemData(hItem, dwId);

	AddEvent(dwEvent);
}


//
// Handler for "Add trap..." button
//

void CCreateMPDlg::OnButtonAddTrap() 
{
	CTrapSelDlg dlg;
	DWORD i;
	
	dlg.m_dwTrapCfgSize = m_dwTrapCfgSize;
	dlg.m_pTrapCfg = m_pTrapCfg;
	if (dlg.DoModal() == IDOK)
	{
      for(i = 0; i < dlg.m_dwNumTraps; i++)
         AddTrap(dlg.m_pdwTrapList[i], dlg.m_pdwEventList[i], dlg.m_ppszNames[i]);
      m_wndTreeCtrl.SortChildren(m_hTrapRoot);
      m_wndTreeCtrl.Expand(m_hTrapRoot, TVE_EXPAND);
      m_wndTreeCtrl.SortChildren(m_hEventRoot);
	}
}


//
// "Delete" button handler
//

void CCreateMPDlg::OnButtonDelete() 
{
   HTREEITEM hItem;

   hItem = m_wndTreeCtrl.GetSelectedItem();
   if (hItem != NULL)
      m_wndTreeCtrl.DeleteItem(hItem);
}


//
// "OK" button handler
//

void CCreateMPDlg::OnOK() 
{
   TCHAR szBuffer[1024];
   HANDLE hFile;

   // Validate file name
   GetDlgItemText(IDC_EDIT_FILE, szBuffer, 1024);
   StrStrip(szBuffer);
   if (szBuffer[0] == 0)
   {
      MessageBox(_T("You must specify valid file name!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
   else
   {
      hFile = CreateFile(szBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
      {
         _tcscpy(szBuffer, _T("Cannot create output file: "));
         GetSystemErrorText(GetLastError(), &szBuffer[27], 990);
         MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
      }
      else
      {
	      CloseHandle(hFile);
         CreateList(m_hEventRoot, &m_dwNumEvents, &m_pdwEventList);
         CreateList(m_hTemplateRoot, &m_dwNumTemplates, &m_pdwTemplateList);
         CreateList(m_hTrapRoot, &m_dwNumTraps, &m_pdwTrapList);
	      CDialog::OnOK();
      }
   }
}


//
// Create output data
//

void CCreateMPDlg::CreateList(HTREEITEM hRoot, DWORD *pdwCount, DWORD **ppdwList)
{
   HTREEITEM hItem;
   DWORD i;

   *pdwCount = 0;
   hItem = m_wndTreeCtrl.GetChildItem(hRoot);
   while(hItem != NULL)
   {
      (*pdwCount)++;
      hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
   }

   if (*pdwCount > 0)
   {
      *ppdwList = (DWORD *)malloc(sizeof(DWORD) * (*pdwCount));
      hItem = m_wndTreeCtrl.GetChildItem(hRoot);
      for(i = 0; (hItem != NULL) && (i < *pdwCount); i++)
      {
         (*ppdwList)[i] = m_wndTreeCtrl.GetItemData(hItem);
         hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
      }
   }
}


//
// "Browse..." button handler
//

void CCreateMPDlg::OnButtonBrowse() 
{
   TCHAR szBuffer[1024];

   GetDlgItemText(IDC_EDIT_FILE, szBuffer, 1024);
   CFileDialog dlg(FALSE, _T("nxmp"), szBuffer, OFN_PATHMUSTEXIST,
                   _T("NetXMS Management Packs (*.nxmp)|*.nxmp|All Files (*.*)|*.*||"));
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
