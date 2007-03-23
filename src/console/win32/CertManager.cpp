// CertManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CertManager.h"
#include "ImportCertDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCertManager

IMPLEMENT_DYNCREATE(CCertManager, CMDIChildWnd)

CCertManager::CCertManager()
{
   m_iSortMode = theApp.GetProfileInt(_T("CertMgr"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("CertMgr"), _T("SortDir"), 0);
	m_pCertList = NULL;
}

CCertManager::~CCertManager()
{
   theApp.WriteProfileInt(_T("CertMgr"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("CertMgr"), _T("SortDir"), m_iSortDir);
	NXCDestroyCertificateList(m_pCertList);
}


BEGIN_MESSAGE_MAP(CCertManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CCertManager)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_CERTIFICATE_IMPORT, OnCertificateImport)
	ON_COMMAND(ID_CERTIFICATE_DELETE, OnCertificateDelete)
	ON_UPDATE_COMMAND_UI(ID_CERTIFICATE_DELETE, OnUpdateCertificateDelete)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCertManager message handlers

BOOL CCertManager::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, GetSysColorBrush(COLOR_WINDOW), 
                                         theApp.LoadIcon(IDI_CERTMGR));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CCertManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
	
   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 3, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_imageList.Add(theApp.LoadIcon(IDI_CERT));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Subject"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, _T("Comments"), LVCFMT_LEFT, 200);
   LoadListCtrlColumns(m_wndListCtrl, _T("CertMgr"), _T("ListCtrl"));

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortDir;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);
	
   theApp.OnViewCreate(VIEW_CERTIFICATE_MANAGER, this);
	
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CCertManager::OnDestroy() 
{
   SaveListCtrlColumns(m_wndListCtrl, _T("CertMgr"), _T("ListCtrl"));
   theApp.OnViewDestroy(VIEW_CERTIFICATE_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CCertManager::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CCertManager::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	m_wndListCtrl.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CCertManager::OnViewRefresh() 
{
	DWORD i, dwResult;
	int nItem;
	TCHAR szBuffer[16];

   m_wndListCtrl.DeleteAllItems();
	NXCDestroyCertificateList(m_pCertList);
	dwResult = DoRequestArg2(NXCGetCertificateList, g_hSession, &m_pCertList, _T("Loading certificate list..."));
	if (dwResult == RCC_SUCCESS)
	{
		for(i = 0; i < m_pCertList->dwNumElements; i++)
		{
			_stprintf(szBuffer, _T("%d"), m_pCertList->pElements[i].dwId);
			nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, 2);
			if (nItem != -1)
			{
				m_wndListCtrl.SetItemData(nItem, i);
				m_wndListCtrl.SetItemText(nItem, 2, m_pCertList->pElements[i].pszSubject);
				m_wndListCtrl.SetItemText(nItem, 3, m_pCertList->pElements[i].pszComments);
			}
		}
	}
	else
	{
		theApp.ErrorBox(dwResult, _T("Error loading certificate list: %s"));
	}
}


//
// WM_COMMAND::ID_CERTIFICATE_IMPORT message handler
//

void CCertManager::OnCertificateImport() 
{
	CImportCertDlg dlg;
	X509 *pCert;
	FILE *fp;
	TCHAR szError[256], szBuffer[512];

	if (dlg.DoModal() == IDOK)
	{
		fp = _tfopen(dlg.m_strFile, _T("r"));
		if (fp != NULL)
		{
			pCert = PEM_read_X509_AUX(fp, NULL, NULL, NULL);
			if (pCert != NULL)
			{
				int nLen;
				BYTE *p, *der;
				DWORD dwResult;

				nLen = i2d_X509(pCert, NULL);
				if (nLen > 0)
				{
					der = (BYTE *)malloc(nLen);
					p = der;
					i2d_X509(pCert, &p);
					dwResult = DoRequestArg4(NXCAddCACertificate, g_hSession, (void *)nLen, der,
					                         (void *)((LPCTSTR)dlg.m_strComments), _T("Importing certificate..."));
					if (dwResult == RCC_SUCCESS)
					{
						PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
					}
					else
					{
						theApp.ErrorBox(dwResult, _T("Error importing certificate: %s"));
					}
					free(der);
				}
				else
				{
					_stprintf(szBuffer, _T("Error encoding certificate:\n%s"),
								 _ERR_error_tstring(ERR_get_error(), szError));
					MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
				}
				X509_free(pCert);
			}
			else
			{
				_stprintf(szBuffer, _T("Error loading certificate from file:\n%s"),
				          _ERR_error_tstring(ERR_get_error(), szError));
				MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
			}
			fclose(fp);
		}
		else
		{
			MessageBox(_T("Cannot open input file"), _T("Error"), MB_OK | MB_ICONSTOP);
		}
	}
}


//
// WM_COMMAND::ID_CERTIFICATE_DELETE message handlers
//

void CCertManager::OnCertificateDelete() 
{
	int i, nCount;

	nCount = m_wndListCtrl.GetSelectedCount();
	if (nCount <= 0)
		return;
}

void CCertManager::OnUpdateCertificateDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// WM_CONTEXTMENU message handler
//

void CCertManager::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(24);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}
