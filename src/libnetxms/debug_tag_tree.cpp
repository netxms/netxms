/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2017 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: debug_tag_tree.cpp
**
**/
#include "debug_tag_tree.h"

/**
 * Create empty tree node
 */
DebugTagTreeNode::DebugTagTreeNode()
{
   m_value = NULL;
   m_direct = false;
   m_directLvL = 0;
   m_asterisk = false;
   m_asteriskLvL = 0;
   m_children = new StringObjectMap<DebugTagTreeNode>(true);
}

/**
 * Create new tree node
 */
DebugTagTreeNode::DebugTagTreeNode(const TCHAR *value, size_t len)
{
   m_value = (TCHAR *)malloc(sizeof(TCHAR) * ((int)len + 1));
   memcpy(m_value, value, sizeof(TCHAR) * (int)len);
   m_value[len] = 0;
   m_direct = false;
   m_directLvL = 0;
   m_asterisk = false;
   m_asteriskLvL = 0;
   m_children = new StringObjectMap<DebugTagTreeNode>(true);
}

/**
 * Get debug LvL from tree node, returns longest match (recursive)
 */
int DebugTagTreeNode::getDebugLvl(const TCHAR *tags)
{
   if (tags == NULL)
   {
      if (m_direct)
         return m_directLvL;
      if (m_asterisk)
         return m_asteriskLvL;
      return -1;
   }

   int result = -1;
   const TCHAR *ptr = _tcschr(tags, _T('.'));
   int len = (ptr == NULL) ? _tcslen(tags) : (ptr - tags);

   DebugTagTreeNode *child = m_children->get(tags, len);
   if (child != NULL)
      result = child->getDebugLvl((ptr != NULL) ? ptr + 1 : NULL);

   if (result == -1 && m_asterisk)
      return m_asteriskLvL;

   return result;
}

/**
 * Add new tree node to its children and grand children etc... (recursive)
 */
void DebugTagTreeNode::add(const TCHAR *tags, UINT32 lvl)
{
   if (tags != NULL && !_tcscmp(tags, _T("*")))
   {
      if (!m_asterisk)
         m_asterisk = true;
      m_asteriskLvL = lvl;
      return;
   }

   const TCHAR *ptr = (tags == NULL) ? NULL : _tcschr(tags, _T('.'));

   if (tags == NULL)
   {
      if (!m_direct)
         m_direct = true;
      m_directLvL = lvl;
      return;
   }

   int len = (ptr == NULL) ? _tcslen(tags) : (ptr - tags);

   DebugTagTreeNode *child = m_children->get(tags, len);
   if (child != NULL)
      child->add((ptr != NULL) ? ptr + 1 : NULL, lvl);
   else
   {
      child = new DebugTagTreeNode(tags, len);
      m_children->set(child->getValue(), child);
      child->add((ptr != NULL) ? ptr + 1 : NULL, lvl);
   }
}

/**
 * Remove entry from child list (recursive)
 */
bool DebugTagTreeNode::remove(const TCHAR *tags)
{
   const TCHAR *ptr = NULL;

   if (tags != NULL)
   {
      ptr = _tcschr(tags, _T('.'));
      int len = (ptr == NULL) ? _tcslen(tags) : (ptr - tags);

      DebugTagTreeNode *child = m_children->get(tags, len);
      if (child != NULL && child->remove((ptr != NULL) ? ptr + 1 : NULL))
         m_children->remove(child->getValue());
   }

   if (tags != NULL && !_tcscmp(tags, _T("*")))
   {
      m_asterisk = false;
      m_asteriskLvL = 0;
   }
   else if (tags == NULL)
   {
      m_direct = false;
      m_directLvL = 0;
   }

   if ((m_children->size() == 0) && !m_asterisk)
      return true;

   return false;
}

/**
 * Get debug LvL from tree, returns longest match (recursive)
 */
UINT32 DebugTagTree::getDebugLvl(const TCHAR *tags)
{
   InterlockedIncrement(&m_readerCount);
   int result;
   if (tags == NULL)
       result = getRootDebugLvl();
    else
    {
       result = m_root->getDebugLvl(tags);
       if (result == -1)
          result = 0;
    }
   InterlockedDecrement(&m_readerCount);
   return result;
}

/**
 * Get main debug level
 */
UINT32 DebugTagTree::getRootDebugLvl()
{
   InterlockedIncrement(&m_readerCount);
   UINT32 level = m_root->m_asteriskLvL;
   InterlockedDecrement(&m_readerCount);
   return level;
}

/**
 * Set debug lvl for root node
 */
void DebugTagTree::setRootDebugLvl(UINT32 lvl)
{
   m_root->m_asteriskLvL = lvl;
   if (!m_root->m_asterisk)
      m_root->m_asterisk = true;
}

/**
 * Add new entry to the tree (recursive)
 */
void DebugTagTree::add(const TCHAR *tags, UINT32 lvl)
{
   if (tags == NULL)
      setRootDebugLvl(lvl);
   else
      m_root->add(tags, lvl);
}

/**
 * Remove entry from the tree
 */
void DebugTagTree::remove(const TCHAR *tags)
{
   if (tags == NULL)
       setRootDebugLvl(0);
    else
       m_root->remove(tags);
}
