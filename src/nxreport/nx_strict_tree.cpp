/*
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nx_strict_tree.h
**
**/

#include <nxclapi.h>
//#include <string.h>

#include "nx_strict_tree.h"

//
// This is called by all construcors to properly initialize defaults.
//
BOOL StrictTree::Init(NXC_OBJECT *pRootObject)
{
  m_rootItem 	= NULL;
  m_currentItem = NULL;
  m_dwCurrentChildNo = 0;
  m_dwCurrentItemIdx = 0;
  m_pTreeIndex = NULL;
  m_dwTreeIndexSize = 0;

  if (pRootObject != NULL)
     return AddItem(pRootObject);  
  else
     return TRUE;
};


//
// "default" constructor - creates tree with root = "All Services"
//		
StrictTree::StrictTree(NXC_SESSION hSession)
{
  m_hSession = hSession;
  Init(NXCGetServiceRootObject(m_hSession));
};


//
// With this constructor You can create subtrees or tree starting from "All Networks"
//
StrictTree::StrictTree(NXC_SESSION hSession, NXC_OBJECT *pRootObject)
{
  m_hSession = hSession;
  Init(pRootObject);
};


//
// Same as previous, but takes object id as parameter, useful if root is pased from command line.
//
StrictTree::StrictTree(NXC_SESSION hSession, DWORD dwObjId)
{
  m_hSession = hSession;
  Init(NXCFindObjectById(m_hSession,dwObjId));
};


//
// We care about about "real" cleanup - as application might need to use this class more than once.
//
StrictTree::~StrictTree()
{
   DWORD dwIdx;

   for (dwIdx = 0; dwIdx < m_dwTreeIndexSize; dwIdx++)
	{
	  if (m_pTreeIndex[dwIdx].pTreeItem->pChildList != NULL) 
		free(m_pTreeIndex[dwIdx].pTreeItem->pChildList);
	  delete m_pTreeIndex[dwIdx].pTreeItem;
	};
    free( m_pTreeIndex);

   return;
};


//
// Easy way to find object in tree. But slow. Realy slow. Need to find a faster way.
//
TREE_ITEM *StrictTree::FindObjectInTree(DWORD dwObjId)
{
  DWORD dwIdx;
  for (dwIdx = 0; dwIdx < m_dwTreeIndexSize; dwIdx++)
	if (m_pTreeIndex[dwIdx].dwObjId == dwObjId) break;

  if (dwIdx < m_dwTreeIndexSize)
	return m_pTreeIndex[dwIdx].pTreeItem;
  else
	return NULL;

};


// 
// May be used to quickly fill in just created tree. 
// Can be used only once and only when tree contains only root object.
//
BOOL StrictTree::LoadTree()
{
  DWORD dwIdx;
  int i;

  if (m_dwTreeIndexSize != 1)
	return FALSE;

  for (dwIdx=0; dwIdx < m_dwTreeIndexSize; dwIdx++)
  {
    for (i=0; i < m_pTreeIndex[dwIdx].pTreeItem->pObject->dwNumChilds; i++)
        AddItem(NXCFindObjectById(m_hSession, m_pTreeIndex[dwIdx].pTreeItem->pObject->pdwChildList[i]));
  };

  // This step is very important. 
  // After this sorting we cant walk throw whole tree by just incrementing current index.
  qsort(m_pTreeIndex, m_dwTreeIndexSize, sizeof(TREE_INDEX_ITEM), &SortCompare);
  return TRUE;
};


//
// Used by LoadTree().  Can be used also to manually add items to tree.
//
BOOL StrictTree::AddItem(NXC_OBJECT *pObject)
{
  int i,j;
  TREE_ITEM *pParent,*pNew, **pChilds;
  BOOL bSucceeded = FALSE;
  TREE_INDEX_ITEM *pTmpIdx;

  if (pObject == NULL) 
	return FALSE;

  // if tree is empty - first object becomes root object.
  if (m_rootItem == NULL)
  {
    pNew = new TREE_ITEM;
    if (pNew==NULL)
	return FALSE;
    pNew->pParent=NULL;
    pNew->pChildList=NULL;
    pNew->dwChildCount=0;
    pNew->dwObjLevel=0;
    pNew->pObject=pObject;
    m_dwTreeIndexSize=1;

    m_rootItem = pNew;
    m_currentItem=m_rootItem;
    bSucceeded=TRUE;
  }
  else // in all other cases object is inserted only if parent is present in tree and don't have this child yet.
  { 
    for (i=0; i < pObject->dwNumParents; i++)
    {
	pParent=FindObjectInTree(pObject->pdwParentList[i]);
	if (pParent!=NULL) // do we have such tree item?
	{
	  for (j = 0; j < pParent->dwChildCount; j++)
		if (pParent->pChildList[j]->pObject->dwId == pObject->dwId)
		   break;
	  if (j==pParent->dwChildCount) // checked all childs of this parent - object not in list.
	  {
            // create new tree item
	    pNew = new TREE_ITEM; 
            pNew->pParent=pParent;
    	    pNew->pChildList=NULL;
    	    pNew->dwChildCount=0;
    	    pNew->dwObjLevel=pParent->dwObjLevel+1;
    	    pNew->pObject=pObject;

	    // register item in parent's child list
	    pChilds= (TREE_ITEM **) realloc( pParent->pChildList ,(pParent->dwChildCount+1) * sizeof(TREE_ITEM *) );
	    if (pChilds!=NULL)
	    {
  	      pParent->pChildList=pChilds;
	      pParent->pChildList[pParent->dwChildCount] = pNew;
              pParent->dwChildCount++;
	      m_dwTreeIndexSize++;
	      bSucceeded = TRUE;
	    }
	    else 
	    { // Failed to register. Rollback.
	      delete pNew;
	      bSucceeded = FALSE;
	    }
	  };
	};
    };
  }

  // if we created new Tree Item - register it also in tree index.
  if (bSucceeded)
  {
    pTmpIdx = (TREE_INDEX_ITEM *) realloc(m_pTreeIndex, sizeof(TREE_INDEX_ITEM) * m_dwTreeIndexSize);
    if (pTmpIdx != NULL) 
    {
	m_pTreeIndex = pTmpIdx;	
        m_pTreeIndex[m_dwTreeIndexSize-1].dwObjId=pObject->dwId;
        m_pTreeIndex[m_dwTreeIndexSize-1].pTreeItem=pNew;
	bSucceeded = TRUE;
    }
    else
    {   // Failed to extend index. Rollback.
	m_dwTreeIndexSize--;
	pNew->pParent->dwChildCount--;
 	// I suppose that realloc to smaller size will always succeed.
	pNew->pParent->pChildList = (TREE_ITEM **) realloc(pNew->pParent->pChildList,pNew->pParent->dwChildCount*sizeof(TREE_ITEM *));
	bSucceeded = FALSE;
    };
  };

  return bSucceeded;
};


//
// Remove one tree item manually.
//
BOOL StrictTree::RemoveItem(NXC_OBJECT *pObject)
{
  return FALSE;
};


//
// Try to prepare for "extra quick tree walk" trick. :) 
//
int StrictTree::SortCompare(const void *pItem1, const void *pItem2)
{
  TREE_ITEM *lpItem1, *lpItem2,*lpItem1Orig, *lpItem2Orig;
   

  lpItem1 = ((TREE_INDEX_ITEM *) pItem1)->pTreeItem;
  lpItem2 = ((TREE_INDEX_ITEM *) pItem2)->pTreeItem;
  lpItem1Orig = lpItem1;
  lpItem2Orig = lpItem2;

  while (lpItem1->dwObjLevel > lpItem2->dwObjLevel) lpItem1=lpItem1->pParent;  	
  while (lpItem2->dwObjLevel > lpItem1->dwObjLevel) lpItem2=lpItem2->pParent;

  if (lpItem1==lpItem2)
	if (lpItem1Orig->dwObjLevel > lpItem2Orig->dwObjLevel)
		return 1;
	else
		return -1;

  while (lpItem1->pParent != lpItem2->pParent) {lpItem1=lpItem1->pParent; lpItem2=lpItem2->pParent; }

  return (_tcscmp(lpItem1->pObject->szName,lpItem2->pObject->szName));
};


//
// Walk over the tree. Each next call returns next object.
//	
NXC_OBJECT *StrictTree::GetNextObject()
{
  DWORD dwUnused;
  return GetNextObject(&dwUnused);
};

NXC_OBJECT *StrictTree::GetNextObject(DWORD *dwLevel)
{
  if (m_dwCurrentItemIdx >= m_dwTreeIndexSize)
	return NULL;

  *dwLevel=m_pTreeIndex[m_dwCurrentItemIdx].pTreeItem->dwObjLevel;
  return m_pTreeIndex[m_dwCurrentItemIdx++].pTreeItem->pObject;
};
