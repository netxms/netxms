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
   m_directLevel = 0;
   m_wildcard = true;
   m_wildcardLevel = 0;
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
   m_directLevel = 0;
   m_wildcard = false;
   m_wildcardLevel = 0;
   m_children = new StringObjectMap<DebugTagTreeNode>(true);
}

/**
 * Get debug LvL from tree node, returns longest match (recursive)
 */
int DebugTagTreeNode::getDebugLevel(const TCHAR *tag) const
{
   if (tag == NULL)
   {
      if (m_direct)
         return m_directLevel;
      if (m_wildcard)
         return m_wildcardLevel;
      return -1;
   }

   int result = -1;
   const TCHAR *ptr = _tcschr(tag, _T('.'));
   size_t len = (ptr == NULL) ? _tcslen(tag) : (ptr - tag);

   DebugTagTreeNode *child = m_children->get(tag, len);
   if (child != NULL)
      result = child->getDebugLevel((ptr != NULL) ? ptr + 1 : NULL);

   if ((result == -1) && m_wildcard)
      return m_wildcardLevel;

   return result;
}

/**
 * Add new tree node to its children and grand children etc... (recursive)
 */
void DebugTagTreeNode::add(const TCHAR *tag, int level)
{
   if ((tag != NULL) && !_tcscmp(tag, _T("*")))
   {
      if (!m_wildcard)
         m_wildcard = true;
      m_wildcardLevel = level;
      return;
   }

   const TCHAR *ptr = (tag == NULL) ? NULL : _tcschr(tag, _T('.'));

   if (tag == NULL)
   {
      if (!m_direct)
         m_direct = true;
      m_directLevel = level;
      return;
   }

   size_t len = (ptr == NULL) ? _tcslen(tag) : (ptr - tag);
   DebugTagTreeNode *child = m_children->get(tag, len);
   if (child != NULL)
      child->add((ptr != NULL) ? ptr + 1 : NULL, level);
   else
   {
      child = new DebugTagTreeNode(tag, len);
      m_children->set(child->getValue(), child);
      child->add((ptr != NULL) ? ptr + 1 : NULL, level);
   }
}

/**
 * Remove entry from child list (recursive)
 */
bool DebugTagTreeNode::remove(const TCHAR *tag)
{
   if (tag != NULL)
   {
      if (!_tcscmp(tag, _T("*")))
      {
         m_wildcard = false;
         m_wildcardLevel = 0;
      }
      else
      {
         const TCHAR *ptr = _tcschr(tag, _T('.'));
         size_t len = (ptr == NULL) ? _tcslen(tag) : (ptr - tag);

         DebugTagTreeNode *child = m_children->get(tag, len);
         if (child != NULL && child->remove((ptr != NULL) ? ptr + 1 : NULL))
         {
            m_children->remove(child->getValue());
         }
      }
   }
   else
   {
      m_direct = false;
      m_directLevel = 0;
   }

   return (m_children->size() == 0) && !m_wildcard;
}

/**
 * Get all tags under this node
 */
void DebugTagTreeNode::getAllTags(const TCHAR *prefix, ObjectArray<DebugTagInfo> *tags) const
{
   TCHAR name[1024];
   _tcslcpy(name, prefix, 1024);
   if (*prefix != 0)
      _tcslcat(name, _T("."), 1024);
   size_t l = _tcslen(name);
   StructArray<KeyValuePair> *c = m_children->toArray();
   for(int i = 0; i < c->size(); i++)
   {
      KeyValuePair *p = c->get(i);
      _tcslcpy(&name[l], p->key, 1024 - l);
      const DebugTagTreeNode *n = static_cast<const DebugTagTreeNode*>(p->value);
      if (n->m_direct)
      {
         tags->add(new DebugTagInfo(name, n->m_directLevel));
      }
      if (n->m_wildcard)
      {
         _tcslcat(name, _T(".*"), 1024);
         tags->add(new DebugTagInfo(name, n->m_wildcardLevel));
         _tcslcpy(&name[l], p->key, 1024 - l);
      }
      n->getAllTags(name, tags);
   }
}

/**
 * Get debug LvL from tree, returns longest match (recursive)
 */
int DebugTagTree::getDebugLevel(const TCHAR *tags)
{
   InterlockedIncrement(&m_readerCount);
   int result;
   if (tags == NULL)
   {
       result = m_root->getWildcardDebugLevel();
   }
   else
   {
      result = m_root->getDebugLevel(tags);
      if (result == -1)
         result = 0;
   }
   InterlockedDecrement(&m_readerCount);
   return result;
}

/**
 * Get main debug level
 */
int DebugTagTree::getRootDebugLevel()
{
   InterlockedIncrement(&m_readerCount);
   int level = m_root->getWildcardDebugLevel();
   InterlockedDecrement(&m_readerCount);
   return level;
}

/**
 * Get all configured tags
 */
ObjectArray<DebugTagInfo> *DebugTagTree::getAllTags()
{
   ObjectArray<DebugTagInfo> *tags = new ObjectArray<DebugTagInfo>(64, 64, true);
   InterlockedIncrement(&m_readerCount);
   m_root->getAllTags(_T(""), tags);
   InterlockedDecrement(&m_readerCount);
   return tags;
}
