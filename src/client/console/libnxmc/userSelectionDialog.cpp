/* $Id$ */
/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2008 Alex Kirhenshtein
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
**/

#include "libnxmc.h"
#include "userSelectionDialog.h"

//
// Event table
//

BEGIN_EVENT_TABLE(nxUserSelectionDialog, wxDialog)
	EVT_INIT_DIALOG(nxUserSelectionDialog::OnInitDialog)
	EVT_LIST_COL_CLICK(XRCID("wndListCtrl"), nxUserSelectionDialog::OnListColumnClick)
	EVT_LIST_ITEM_ACTIVATED(XRCID("wndListCtrl"), nxUserSelectionDialog::OnListItemActivate)
END_EVENT_TABLE()


//
// Constructor
//

nxUserSelectionDialog::nxUserSelectionDialog(wxWindow *parent)
			: wxDialog()
{
	wxConfig::Get()->Read(_T("/UserSelDlg/SortMode"), &m_sortMode, 0);
	wxConfig::Get()->Read(_T("/UserSelDlg/SortDir"), &m_sortDir, 1);

	wxXmlResource::Get()->LoadDialog(this, parent, _T("nxUserSelDlg"));
	GetSizer()->Fit(this);

	m_showPublic = false;
	m_showGroups = true;
	m_singleSelection = true;

	m_selectedUsersCount = 0;
	m_selectedUsers = NULL;
}


//
// Destructor
//

nxUserSelectionDialog::~nxUserSelectionDialog()
{
	wxConfig::Get()->Write(_T("/UserSelDlg/SortMode"), m_sortMode);
	wxConfig::Get()->Write(_T("/UserSelDlg/SortDir"), m_sortDir);

	wxListCtrl *wndListCtrl = XRCCTRL(*this, "wndListCtrl", wxListCtrl);
	NXMCSaveListCtrlColumns(wxConfig::Get(), *wndListCtrl, _T("/UserSelDlg/UserList"));

	wxSize size = this->GetSize();
	wxConfig::Get()->Write(_T("/UserSelDlg/SizeX"), size.GetX());
	wxConfig::Get()->Write(_T("/UserSelDlg/SizeY"), size.GetY());

	if (m_selectedUsers != NULL)
	{
		delete m_selectedUsers;
		m_selectedUsers = NULL;
	}
}


//
// Setters
//

void nxUserSelectionDialog::SetShowGroups(bool flag)
{
	m_showGroups = flag;
}

void nxUserSelectionDialog::SetShowPublic(bool flag)
{
	m_showPublic = flag;
}

void nxUserSelectionDialog::SetSingleSelection(bool flag)
{
	m_singleSelection = flag;
}

// Getters

DWORD nxUserSelectionDialog::GetSelectedUsersCount()
{
	return m_selectedUsersCount;
}

DWORD *nxUserSelectionDialog::GetSelectedUsers()
{
	return m_selectedUsers;
}

//
// Data exchange
//

bool nxUserSelectionDialog::TransferDataFromWindow(void)
{
	wxListCtrl *list = XRCCTRL(*this, "wndListCtrl", wxListCtrl);

	m_selectedUsersCount = list->GetSelectedItemCount();
	m_selectedUsers = new DWORD[m_selectedUsersCount];

	long item = -1;
	int i = 0;
	do 
	{
		item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item != -1)
		{
			m_selectedUsers[i++] = (DWORD)list->GetItemData(item);
		}
	} while (item != -1);

	return true;
}

bool nxUserSelectionDialog::TransferDataToWindow(void)
{
	return true;
}


//
// Object comparison callback
//

static int wxCALLBACK CompareObjects(long item1, long item2, long sortData)
{
	int rc = 0;

	if (((item1 & GROUP_FLAG) && (item2 & GROUP_FLAG)) || (!(item1 & GROUP_FLAG) && !(item2 & GROUP_FLAG)))
	{
		rc = ((unsigned long)item1) - ((unsigned long)item2);
	}
	else if (item1 & GROUP_FLAG)
	{
		rc = -1;
	}
	else if (item2 & GROUP_FLAG)
	{
		rc = 1;
	}


	return rc;
}

//
// Dialog initialization handler
//

void nxUserSelectionDialog::OnInitDialog(wxInitDialogEvent &event)
{
	wxListCtrl *list = XRCCTRL(*this, "wndListCtrl", wxListCtrl);

	// cleanup it not first invocation
	m_selectedUsersCount = 0;
	if (m_selectedUsers != NULL)
	{
		delete m_selectedUsers;
		m_selectedUsers = NULL;
	}
	list->ClearAll();

	// set selection type
	list->SetSingleStyle(wxLC_SINGLE_SEL, m_singleSelection);

	// load and assign icons
	wxImageList *imageList = new wxImageList(16, 16);

	imageList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallUser")));
	imageList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallUserGroup")));
	imageList->Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallEveryone")));
	list->AssignImageList(imageList, wxIMAGE_LIST_SMALL);

	// load and set dialog size
	int sizeX, sizeY;
	wxConfig::Get()->Read(_T("/UserSelDlg/SizeX"), &sizeX, 400);
	wxConfig::Get()->Read(_T("/UserSelDlg/SizeY"), &sizeY, 300);

	this->SetMinSize(wxSize(300, 200));
	this->SetSize(wxSize(sizeX, sizeY));
	this->Layout();
	
	// add columns
	int width = list->GetClientSize().GetX();
	long rc = list->InsertColumn(0, _T("Name"), wxLIST_FORMAT_LEFT, 200);
printf("user: rc1=%d\n",rc);	
	rc = list->InsertColumn(1, _T("Full Name"), wxLIST_FORMAT_LEFT, width - 200 - wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
printf("user: rc2=%d\n",rc);	

	NXMCLoadListCtrlColumns(wxConfig::Get(), *list, _T("/UserSelDlg/UserList"));
	
	// Populate list
	NXC_USER *userList;
	DWORD i, userCount;
	//NXCLockUserDB(NXMCGetSession());

	if (m_showPublic)
	{
		int item = list->InsertItem(0x7FFFFFFF, _T("[public]"), 2);
		list->SetItemData(item, GROUP_EVERYONE);
	}
	if (NXCGetUserDB(NXMCGetSession(), &userList, &userCount))
	{
		for(i = 0; i < userCount; i++)
		{
			if (userList[i].wFlags & UF_DELETED)
			{
				continue;
			}

			if (!m_showGroups && (userList[i].dwId & GROUP_FLAG))
			{
				continue;
			}

			//if (!(userList[i].dwId & GROUP_FLAG) && !(userList[i].wFlags & UF_DELETED))
			int imageIndex;
			if (userList[i].dwId == GROUP_EVERYONE)
			{
				imageIndex = 2;
			}
			else
			{
				if (userList[i].dwId & GROUP_FLAG)
				{
					imageIndex = 1;
				}
				else
				{
					imageIndex = 0;
				}
			}

			long item = list->InsertItem(0x7FFFFFFF, userList[i].szName, imageIndex);
			list->SetItem(item, 1, userList[i].szFullName);
			list->SetItemData(item, userList[i].dwId);
		}
	}
	//NXCUnlockUserDB(NXMCGetSession());

	list->SortItems(CompareObjects, 0);

	// Set focus to "OK"
	wxWindow *wnd;

	wnd = FindWindowById(wxID_OK, this);
	if (wnd != NULL)
	{
		((wxButton *)wnd)->SetDefault();
	}
	else
	{
		wxLogDebug(_T("nxUserSelectionDialog::OnInitDialog(): cannot find child with id wxID_OK"));
	}

	event.Skip();
}


//
// Handler for column click
//

void nxUserSelectionDialog::OnListColumnClick(wxListEvent &event)
{

}


//
// Handler for item activate (Enter or Double click)
//

void nxUserSelectionDialog::OnListItemActivate(wxListEvent &event)
{
	if (Validate() && TransferDataFromWindow())
	{
		EndDialog(wxID_OK);
	}
}
