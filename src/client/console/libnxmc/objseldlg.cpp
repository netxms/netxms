/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: objseldlg.cpp
**
**/

#include "libnxmc.h"
#include "objseldlg.h"


//
// Event table
//

BEGIN_EVENT_TABLE(nxObjectSelDlg, wxDialog)
	EVT_INIT_DIALOG(nxObjectSelDlg::OnInitDialog)
	EVT_LIST_COL_CLICK(XRCID("wndListCtrl"), nxObjectSelDlg::OnListColumnClick)
	EVT_LIST_ITEM_ACTIVATED(XRCID("wndListCtrl"), nxObjectSelDlg::OnListItemActivate)
END_EVENT_TABLE()


//
// Constructor
//

nxObjectSelDlg::nxObjectSelDlg(wxWindow *parent)
			: wxDialog()
{
	wxConfig::Get()->Read(_T("/ObjectSelDlg/SortMode"), &m_sortMode, 0);
	wxConfig::Get()->Read(_T("/ObjectSelDlg/SortDir"), &m_sortDir, 1);
	
	m_parentObjectId = 0;
	m_allowedClasses = SCL_NODE;
	m_isSelectAddress = false;
	m_isShowLoopback = false;
	m_isSingleSelection = true;
	m_isEmptySelectionAllowed = false;
	m_customAttributeFilter = NULL;
	m_customAttributeFilterInverted = false;
	
	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxObjectSelDlg"));
	GetSizer()->Fit(this);
}


//
// Destructor
//

nxObjectSelDlg::~nxObjectSelDlg()
{
	wxConfig::Get()->Write(_T("/ObjectSelDlg/SortMode"), m_sortMode);
	wxConfig::Get()->Write(_T("/ObjectSelDlg/SortDir"), m_sortDir);
}


//
// Data exchange
//

bool nxObjectSelDlg::TransferDataFromWindow(void)
{
	wxListCtrl *wndListCtrl;
	int i, count;
	long item;
	
	wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	count = wndListCtrl->GetSelectedItemCount();
	for(i = 0, item = -1; i < count;i++)
	{
		item = wndListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		m_objectList.Add(wndListCtrl->GetItemData(item));
	}
	return true;
}

bool nxObjectSelDlg::TransferDataToWindow(void)
{
	wxListCtrl *wndListCtrl;
	int width;
	DWORD i, numObjects;
	NXC_OBJECT_INDEX *index;
	long item;
	static DWORD classMask[15] = { 0, SCL_SUBNET, SCL_NODE, SCL_INTERFACE,
									SCL_NETWORK, SCL_CONTAINER, SCL_ZONE,
									SCL_SERVICEROOT, SCL_TEMPLATE, SCL_TEMPLATEGROUP,
									SCL_TEMPLATEROOT, SCL_NETWORKSERVICE,
									SCL_VPNCONNECTOR, SCL_CONDITION, SCL_CLUSTER };
	
	wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	if (wndListCtrl == NULL)
	{
		wxLogError(_T("INTERNAL ERROR: wndListCtrl is NULL in nxObjectSelDlg::TransferDataToWindow"));
		return false;
	}
	
	// Fill in object list
	if (m_parentObjectId == 0)
	{
		NXCLockObjectIndex(NXMCGetSession());
		index = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(NXMCGetSession(), &numObjects);
		int __counter = 0;
		for(i = 0; i < numObjects; i++)
		{
			if ((classMask[index[i].object->iClass] & m_allowedClasses) &&
				(!index[i].object->bIsDeleted))
			{
				if (m_customAttributeFilter != NULL)
				{
					// TODO: Clean this code later; I can't think right now.
					TCHAR *customAttribute = index[i].object->pCustomAttrs->Get(m_customAttributeFilter);
					if (!m_customAttributeFilterInverted)
					{
						if (customAttribute == NULL)
						{
							continue;
						}
					}
					else
					{
						if (customAttribute != NULL)
						{
							continue;
						}
					}
				}/*
printf("list=%p %d insert %p name=%p (%S) class=%d\n",wndListCtrl, i,index[i].object,index[i].object->szName,index[i].object->szName,index[i].object->iClass);
				item = wndListCtrl->InsertItem(0x7FFFFFFF, index[i].object->szName,
					index[i].object->iClass);
printf("%d item=%d\n",i,item);
				wndListCtrl->SetItem(item, 1, NXMCGetClassName(index[i].object->iClass));
				wndListCtrl->SetItemData(item, index[i].object->dwId);*/
			}
		}
		NXCUnlockObjectIndex(NXMCGetSession());
	}
	else
	{
		NXC_OBJECT *object, *child;
		TCHAR buffer[16];
		
		object = NXCFindObjectById(NXMCGetSession(), m_parentObjectId);
		if (object != NULL)
		{
			for(i = 0; i < object->dwNumChilds; i++)
			{
				child = NXCFindObjectById(NXMCGetSession(), object->pdwChildList[i]);
				if (child != NULL)
				{
					if (m_isSelectAddress)
					{
						if ((child->iClass == OBJECT_INTERFACE) &&
							(child->dwIpAddr != 0) && (!child->bIsDeleted))
						{
							item = wndListCtrl->InsertItem(0x7FFFFFFF, child->szName, OBJECT_INTERFACE);
							wndListCtrl->SetItem(item, 1, IpToStr(child->dwIpAddr, buffer));
							wndListCtrl->SetItemData(item, object->pdwChildList[i]);
						}
					}
					else
					{
						if ((classMask[child->iClass] & m_allowedClasses) &&
							(!child->bIsDeleted))
						{
							item = wndListCtrl->InsertItem(0x7FFFFFFF, child->szName, child->iClass);
							wndListCtrl->SetItem(item, 1, NXMCGetClassName(child->iClass));
							wndListCtrl->SetItemData(item, object->pdwChildList[i]);
						}
					}
				}
			}
			
			if (m_isSelectAddress && m_isShowLoopback)
			{
				item = wndListCtrl->InsertItem(0x7FFFFFFF, _T("<loopback>"), OBJECT_INTERFACE);
				wndListCtrl->SetItem(item, 1, _T("127.0.0.1"));
				wndListCtrl->SetItemData(item, 0);
			}
		}
	}
	
	SortObjects();
	
	// Disable "None" button if empty selection is not allowed
	if (!m_isEmptySelectionAllowed)
	{
		wxButton *btn = XRCCTRL(*this, "btnNone", wxButton);
		if (btn != NULL)
			btn->Hide();
	}
	
	return true;
}


//
// Dialog initialization handler
//

void nxObjectSelDlg::OnInitDialog(wxInitDialogEvent &event)
{
	wxWindow *wnd;
	wxListCtrl *wndListCtrl;
	int width;
	
	wnd = FindWindowById(wxID_OK, this);
	if (wnd != NULL)
		((wxButton *)wnd)->SetDefault();
	else
		wxLogDebug(_T("nxObjectSelDlg::OnInitDialog(): cannot find child with id wxID_OK"));

	wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	if (wndListCtrl == NULL)
	{
		wxLogError(_T("INTERNAL ERROR: wndListCtrl is NULL in nxObjectSelDlg::OnInitDialog"));
		return;
	}
	
	wxImageList *imgList = NXMCGetImageListCopy(IMAGE_LIST_OBJECTS_SMALL);
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortUp")));
	imgList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSortDown")));
	wndListCtrl->AssignImageList(imgList, wxIMAGE_LIST_SMALL);
	
	width = wndListCtrl->GetClientSize().GetX();
printf("width=%d\n",width);
	if (m_isSelectAddress)
	{
		wndListCtrl->InsertColumn(0, _T("Interface"), wxLIST_FORMAT_LEFT,
			width - 150 - wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
		wndListCtrl->InsertColumn(1, _T("IP Address"), wxLIST_FORMAT_LEFT, 150);
	}
	else
	{
long rc;
		rc = wndListCtrl->InsertColumn(0, _T("Name"), wxLIST_FORMAT_LEFT,
			width - 100 - wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
printf("rc1=%d, cols=%d\n",rc,wndListCtrl->GetColumnCount());		
		rc = wndListCtrl->InsertColumn(1, _T("Class"), wxLIST_FORMAT_LEFT, 100);
printf("rc2=%d col=%d\n",rc,wndListCtrl->GetColumnCount());		
	}
	if (m_isSingleSelection)
	{
		wndListCtrl->SetSingleStyle(wxLC_SINGLE_SEL);
	}


	event.Skip();
}


//
// Object comparision callback
//

static int wxCALLBACK CompareObjects(long item1, long item2, long sortData)
{
	long mode = sortData & 0x00FF;
	int rc;
	TCHAR name1[MAX_OBJECT_NAME], name2[MAX_OBJECT_NAME];
	NXC_OBJECT *obj1, *obj2;
	
	switch(mode)
	{
	case 0:	// Name
		NXCGetComparableObjectName(NXMCGetSession(), item1, name1);
		NXCGetComparableObjectName(NXMCGetSession(), item2, name2);
		rc = _tcsicmp(name1, name2);
		break;
	case 1:	// Class or IP address
		obj1 = NXCFindObjectById(NXMCGetSession(), item1);
		obj2 = NXCFindObjectById(NXMCGetSession(), item2);
		if (sortData & 0x4000)	// IP address
		{
			rc = COMPARE_NUMBERS(obj1->dwIpAddr, obj2->dwIpAddr);
		}
		else
		{
			rc = _tcsicmp(NXMCGetClassName(obj1->iClass), NXMCGetClassName(obj2->iClass));
		}
		break;
	default:
		rc = 0;
		break;
	}
	
	return (sortData & 0x8000) ? rc : -rc;
}


//
// Sort objects and mark sorting column
//

void nxObjectSelDlg::SortObjects()
{
	wxListCtrl *wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	wndListCtrl->SortItems(CompareObjects, m_sortMode | ((m_sortDir == 1) ? 0x8000 : 0) | (m_isSelectAddress ? 0x4000 : 0));
	
	wxListItem col;
//	col.SetColumn(m_sortMode);
	col.SetMask(wxLIST_MASK_IMAGE);
	col.SetImage(OBJECT_CLUSTER + ((m_sortDir == 1) ? 1 : 2));
	wndListCtrl->SetColumn(m_sortMode, col);
}


//
// Handler for column click
//

void nxObjectSelDlg::OnListColumnClick(wxListEvent &event)
{
	wxListCtrl *wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	
//	wndListCtrl->ClearColumnImage(m_sortMode);
	wxListItem col;
//	col.SetColumn(m_sortMode);
	col.SetMask(wxLIST_MASK_IMAGE);
	col.SetImage(-1);
	wndListCtrl->SetColumn(m_sortMode, col);
	
	if (m_sortMode == event.GetColumn())
	{
		m_sortDir = -m_sortDir;
	}
	else
	{
		m_sortMode = event.GetColumn();
	}
	SortObjects();
}


//
// Handler for item activate (Enter or Double click)
//

void nxObjectSelDlg::OnListItemActivate(wxListEvent &event)
{
	if (Validate() && TransferDataFromWindow())
	{
		EndDialog(wxID_OK);
	}
}
