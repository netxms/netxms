/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
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
** File: browser.cpp
**
**/

#include "object_browser.h"


//
// Implementation of dynamic array of tree item references
//

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(nxTreeItemList);


//
// Event table
//

BEGIN_EVENT_TABLE(nxObjectBrowser, nxView)
	EVT_SIZE(nxObjectBrowser::OnSize)
	EVT_NX_REFRESH_VIEW(nxObjectBrowser::OnViewRefresh)
	EVT_TREE_ITEM_EXPANDING(wxID_TREE_CTRL, nxObjectBrowser::OnTreeItemExpanding)
	EVT_TREE_SEL_CHANGED(wxID_TREE_CTRL, nxObjectBrowser::OnTreeSelChanged)
	EVT_TREE_DELETE_ITEM(wxID_TREE_CTRL, nxObjectBrowser::OnTreeDeleteItem)
	EVT_TREE_ITEM_MENU(wxID_TREE_CTRL, nxObjectBrowser::OnTreeItemMenu)
END_EVENT_TABLE()


//
// Constructor
//

nxObjectBrowser::nxObjectBrowser()
                : nxView(NXMCGetDefaultParent())
{
	SetName(_T("objectbrowser"));
	SetLabel(_T("Object Browser"));
	SetIcon(wxXmlResource::Get()->LoadIcon(_T("icoObjectBrowser")));
	m_wndSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER);
	m_wndSplitter->SetMinimumPaneSize(30);
	m_wndTreeCtrl = new wxTreeCtrl(m_wndSplitter, wxID_TREE_CTRL, wxDefaultPosition, wxDefaultSize,
	                               wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT);
	m_wndTreeCtrl->SetImageList(NXMCGetImageList(IMAGE_LIST_OBJECTS_SMALL));
	m_wndObjectView = new nxObjectView(m_wndSplitter);
	m_wndSplitter->SplitVertically(m_wndTreeCtrl, m_wndObjectView, 250);
	RegisterUniqueView(_T("objectbrowser"), this);
	m_isFirstResize = true;

	wxCommandEvent event(nxEVT_REFRESH_VIEW);
	AddPendingEvent(event);

	NXMCEvtConnect(nxEVT_NXC_OBJECT_CHANGE, wxCommandEventHandler(nxObjectBrowser::OnObjectChange), this);
}


//
// Destructor
//

nxObjectBrowser::~nxObjectBrowser()
{
	NXMCEvtDisconnect(nxEVT_NXC_OBJECT_CHANGE, wxCommandEventHandler(nxObjectBrowser::OnObjectChange), this);
	UnregisterUniqueView(_T("objectbrowser"));
}


//
// Resize handler
//

void nxObjectBrowser::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_wndSplitter->SetSize(0, 0, size.x, size.y);
	if (m_isFirstResize)
	{
		m_wndSplitter->SetSashPosition(250);
		m_isFirstResize = false;
	}
}


//
// View->Refresh menu handler
//

void nxObjectBrowser::OnViewRefresh(wxCommandEvent &event)
{
   NXC_OBJECT **ppRootObjects;
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, j, dwNumObjects, dwNumRootObj;
	wxTreeItemId root;
   
   // Select root objects
   NXCLockObjectIndex(NXMCGetSession());
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(NXMCGetSession(), &dwNumObjects);
   ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
   for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
      if (!pIndex[i].object->bIsDeleted)
      {
         // Check if some of the parents are accessible
         for(j = 0; j < pIndex[i].object->dwNumParents; j++)
            if (NXCFindObjectByIdNoLock(NXMCGetSession(), pIndex[i].object->pdwParentList[j]) != NULL)
               break;
         if (j == pIndex[i].object->dwNumParents)
         {
            // No accessible parents or no parents at all
            ppRootObjects[dwNumRootObj++] = pIndex[i].object;
         }
      }
   NXCUnlockObjectIndex(NXMCGetSession());

   // Populate objects' tree
	m_wndTreeCtrl->DeleteAllItems();
	root = m_wndTreeCtrl->AddRoot(_T("[root]"));
	ClearObjectItemsHash();

   for(i = 0; i < dwNumRootObj; i++)
      AddObjectToTree(ppRootObjects[i], root);
   safe_free(ppRootObjects);
}


//
// Add object to tree
//

void nxObjectBrowser::AddObjectToTree(NXC_OBJECT *object, wxTreeItemId &root)
{
	wxTreeItemId item;
	nxObjectItemsHash::iterator it;
	nxTreeItemList *list;
	
	item = m_wndTreeCtrl->AppendItem(root, object->szName, object->iClass, -1, new nxObjectTreeItemData(object));
	
	// Add item to hash
	it = m_objectItemsHash.find(object->dwId);
	if (it != m_objectItemsHash.end())
	{
		list = it->second;
	}
	else
	{
		list = new nxTreeItemList;
		m_objectItemsHash[object->dwId] = list;
	}
	list->Add(item);

   // Don't add childs immediatelly to
   // prevent adding millions of items if node has thousands of interfaces in
   // thousands subnets. Childs will be added only if user expands node.
	m_wndTreeCtrl->SetItemHasChildren(item, object->dwNumChilds > 0);
}


//
// Handler for object tree item expansion
//

void nxObjectBrowser::OnTreeItemExpanding(wxTreeEvent &event)
{
	wxTreeItemId item, temp;
	NXC_OBJECT *object, *childObject;
	DWORD i;
	
	item = event.GetItem();
	temp = m_wndTreeCtrl->GetLastChild(item);
	if (m_wndTreeCtrl->ItemHasChildren(item) && !temp.IsOk())
	{
		object = ((nxObjectTreeItemData *)m_wndTreeCtrl->GetItemData(item))->GetObject();
      for(i = 0; i < object->dwNumChilds; i++)
      {
         childObject = NXCFindObjectById(NXMCGetSession(), object->pdwChildList[i]);
         if (childObject != NULL)
         {
            AddObjectToTree(childObject, item);
         }
      }
		m_wndTreeCtrl->SortChildren(item);
	}
}


//
// Handler for object tree selection change
//

void nxObjectBrowser::OnTreeSelChanged(wxTreeEvent &event)
{
	wxTreeItemId item;

	item = event.GetItem();
	m_wndObjectView->SetObject(((nxObjectTreeItemData *)m_wndTreeCtrl->GetItemData(item))->GetObject());
	m_wndTreeCtrl->SetFocus();   // Return focus to tree control
}


//
// Handler for tree item deletion
//

void nxObjectBrowser::OnTreeDeleteItem(wxTreeEvent &event)
{
	wxTreeItemId item;
	NXC_OBJECT *object;
	nxObjectItemsHash::iterator it;
	nxTreeItemList *list;
	size_t i, count;

	// Find item being deleted in object items hash and remove
	item = event.GetItem();
	object = ((nxObjectTreeItemData *)m_wndTreeCtrl->GetItemData(item))->GetObject();
	it = m_objectItemsHash.find(object->dwId);
	list = (it != m_objectItemsHash.end()) ? it->second : NULL;
	if (list != NULL)
	{
		count = list->GetCount();
		for(i = 0; i < count; i++)
		{
			if (list->Item(i) == item)
			{
				list->RemoveAt(i);
				break;
			}
		}
	}
}


//
// Handler for object change events
//

void nxObjectBrowser::OnObjectChange(wxCommandEvent &event)
{
	NXC_OBJECT *object = (NXC_OBJECT *)event.GetClientData();
	nxObjectItemsHash::iterator it;
	nxTreeItemList *list;
	size_t i, count;

	it = m_objectItemsHash.find(object->dwId);
	list = (it != m_objectItemsHash.end()) ? it->second : NULL;

	if (object->bIsDeleted)
	{
		if (list != NULL)
		{
			count = list->GetCount();
			for(i = 0; i < count; i++)
				m_wndTreeCtrl->Delete(list->Item(i));
			delete list;
			m_objectItemsHash.erase(object->dwId);
		}
	}
	else
	{
		DWORD j;
		
		if (list != NULL)    // Check object's parents
		{
			wxTreeItemId item;
			NXC_OBJECT *parent;

			// Update "hasChildren" attribute
			count = list->GetCount();
			for(i = 0; i < count; i++)
			{
				m_wndTreeCtrl->SetItemHasChildren(list->Item(i), object->dwNumChilds > 0);
			}
			
			// Create a copy of object's parent list
			DWORD *parentList = (DWORD *)nx_memdup(object->pdwParentList, 
			                                       sizeof(DWORD) * object->dwNumParents);

restart_check:
			count = list->GetCount();
			for(i = 0; i < count; i++)
			{
            // Check if this item's parent still in object's parents list
				item = m_wndTreeCtrl->GetItemParent(list->Item(i));
				if (item.IsOk() && (item != m_wndTreeCtrl->GetRootItem()))
				{
					parent = ((nxObjectTreeItemData *)m_wndTreeCtrl->GetItemData(item))->GetObject();
               for(j = 0; j < object->dwNumParents; j++)
                  if (object->pdwParentList[j] == parent->dwId)
                  {
                     parentList[j] = 0;   // Mark this parent as presented
                     break;
                  }
               if (j == object->dwNumParents)  // Not a parent anymore
               {
						m_wndTreeCtrl->Delete(list->Item(i));
						goto restart_check;
					}
					else  // Current tree item is still valid
					{
						m_wndTreeCtrl->SetItemText(list->Item(i), object->szName);
						m_wndTreeCtrl->SortChildren(item);
					}
				}
            else  // Current tree item has no parent
				{
					m_wndTreeCtrl->SetItemText(list->Item(i), object->szName);
				}
			}
			
			// Now walk through all object's parents which hasn't corresponding
			// items in tree view
			for(j = 0; j < object->dwNumParents; j++)
			{
				if (parentList[j] != 0)
				{
					it = m_objectItemsHash.find(parentList[j]);
					if (it != m_objectItemsHash.end())
               {
                  // Walk through all occurences of current parent object
	               list = it->second;
						count = list->GetCount();
						for(i = 0; i < count; i++)
						{
							if (m_wndTreeCtrl->ItemHasChildren(list->Item(i)) && m_wndTreeCtrl->GetLastChild(list->Item(i)).IsOk())
                     {
                        AddObjectToTree(object, list->Item(i));
	                     m_wndTreeCtrl->SortChildren(list->Item(i));
							}
						}
					}
				}
			}

			// Destroy copy of object's parents list
			safe_free(parentList);
		}
		else
		{
			// New object, link to all parents
			for(j = 0; j < object->dwNumParents; j++)
			{
				it = m_objectItemsHash.find(object->pdwParentList[j]);
				if (it != m_objectItemsHash.end())
            {
               // Walk through all occurences of current parent object
               list = it->second;
					count = list->GetCount();
					for(i = 0; i < count; i++)
               {
                  if (m_wndTreeCtrl->ItemHasChildren(list->Item(i)) && m_wndTreeCtrl->GetLastChild(list->Item(i)).IsOk())
                  {
                     AddObjectToTree(object, list->Item(i));
                     m_wndTreeCtrl->SortChildren(list->Item(i));
                  }
               }
            }
         }
      }
	}
}


//
// Clear has of object items
//

void nxObjectBrowser::ClearObjectItemsHash()
{
	nxObjectItemsHash::iterator it;

	for(it = m_objectItemsHash.begin(); it != m_objectItemsHash.end(); it++)
		delete it->second;
	m_objectItemsHash.clear();
}


//
// Handler for context menu on tree item
//

void nxObjectBrowser::OnTreeItemMenu(wxTreeEvent &event)
{
	wxTreeItemId item;
	NXC_OBJECT *object;
	wxMenu *menu;

	if (!IsBusy())
	{
		item = event.GetItem();
		m_wndTreeCtrl->SelectItem(item);
		object = ((nxObjectTreeItemData *)m_wndTreeCtrl->GetItemData(item))->GetObject();
		menu = wxXmlResource::Get()->LoadMenu(_T("menuCtxObject"));
		if (menu != NULL)
		{
			PopupMenu(menu);
			delete menu;
		}
	}
}

