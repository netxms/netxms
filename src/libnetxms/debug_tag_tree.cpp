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
 * Create new tree node
 */
DebugTagTreeNode::DebugTagTreeNode(const TCHAR *value)
{
   m_value = new String(value);
   m_direct = false;
   m_directLvL = 0;
   m_asterisk = false;
   m_asteriskLvL = 0;
   m_children = new StringObjectMap<DebugTagTreeNode>(true);
}

/**
 * Get debug LvL from tree node, returns longest match (recursive)
 */
int DebugTagTreeNode::getDebugLvl(const StringList *tags, UINT32 pos)
{
   if ((tags->size() == pos) && m_direct)
      return m_directLvL;

   int result = -1;
   DebugTagTreeNode *child = m_children->get(tags->get(pos));
   if (child != NULL)
      result = child->getDebugLvl(tags, pos+1);

   if (result == -1 && tags->size() > pos && m_asterisk)
      return m_asteriskLvL;

   return result;
}

/**
 * Add new tree node to its children and grand children etc... (recursive)
 */
void DebugTagTreeNode::add(const StringList *tags, UINT32 pos, UINT32 lvl)
{
   if (tags->size() == pos)
   {
      if (!m_direct)
         m_direct = true;
      m_directLvL = lvl;

      return;
   }

   DebugTagTreeNode *child = m_children->get(tags->get(pos));
   if (child != NULL)
      child->add(tags, pos+1, lvl);
   else
   {
      if (tags->size() > pos && !_tcscmp(tags->get(pos), _T("*")))
      {
         if (!m_asterisk)
            m_asterisk = true;
         m_asteriskLvL = lvl;
      }
      else
      {
         child = new DebugTagTreeNode(tags->get(pos));
         m_children->set(tags->get(pos), child);
         child->add(tags, pos+1, lvl);
      }
   }
}

/**
 * Remove entry from child list (recursive)
 */
bool DebugTagTreeNode::remove(const StringList *tags, UINT32 pos)
{
   DebugTagTreeNode *child = m_children->get(tags->get(pos));
   if (child != NULL && child->remove(tags, pos+1))
      m_children->remove(tags->get(pos));

   if ((tags->size() == pos+1) && (!_tcscmp(tags->get(pos), _T("*"))))
   {
      m_asterisk = false;
      m_asteriskLvL = -1;
   }
   else if ((tags->size() == pos) && m_direct)
   {
      m_direct = false;
      m_directLvL = -1;
   }

   if ((m_children->size() == 0) && !m_asterisk)
      return true;

   return false;
}

/**
 * Get debug LvL from tree, returns longest match (recursive)
 */
int DebugTagTree::getDebugLvl(const TCHAR *tags)
{
   String s(tags);
   StringList *tagList = s.split(_T("."));
   if (tagList->size() == 0)
      tagList->add(_T("*"));

   int result = m_root->getDebugLvl(tagList, 0);

   delete(tagList);
   if (result == -1)
      return 0;
   return result;
}

/**
 * Add new entry to the tree (recursive)
 */
void DebugTagTree::add(const TCHAR *tags, UINT32 lvl)
{
   String s(tags);
   StringList *tagList = s.split(_T("."));
   if (tagList->size() == 0)
      tagList->add(_T("*"));

   m_root->add(tagList, 0, lvl);

   delete(tagList);
}

/**
 * Remove entry from the tree
 */
void DebugTagTree::remove(const TCHAR *tags)
{
   String s(tags);
   StringList *tagList = s.split(_T("."));
   if (tagList->size() == 0)
      tagList->add(_T("*"));

   m_root->remove(tagList, 0);
}
