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
** File: debug_tag_tree.h
**
**
*/

#ifndef _debug_tag_tree_h_
#define _debug_tag_tree_h_

#include <nms_util.h>

/**
 * Node of debug tag tree
 */
class DebugTagTreeNode
{
private:
   TCHAR *m_value;
   StringObjectMap<DebugTagTreeNode> *m_children;
   bool m_direct;
   bool m_wildcard;
   int m_directLevel;
   int m_wildcardLevel;

public:
   DebugTagTreeNode();
   DebugTagTreeNode(const TCHAR *value, size_t len);
   ~DebugTagTreeNode() { free(m_value); delete m_children; }

   int getDebugLevel(const TCHAR *tag) const;
   int getWildcardDebugLevel() const { return m_wildcardLevel; }
   const TCHAR *getValue() const { return m_value; }
   void getAllTags(const TCHAR *prefix, ObjectArray<DebugTagInfo> *tags) const;

   void add(const TCHAR *tag, int level);
   bool remove(const TCHAR *tag);
   void setWildcardDebugLevel(int level) { m_wildcardLevel = level; }
};

/**
 * Debug tag tree
 */
class DebugTagTree
{
private:
   DebugTagTreeNode *m_root;
   VolatileCounter m_readerCount;

public:
   DebugTagTree() { m_root = new DebugTagTreeNode(); m_readerCount = 0; }
   ~DebugTagTree() { delete m_root; }

   int getDebugLevel(const TCHAR *tags);
   int getRootDebugLevel();
   int getReaderCount() { return (int)m_readerCount; }
   ObjectArray<DebugTagInfo> *getAllTags();

   void add(const TCHAR *tag, int level) { m_root->add(tag, level); }
   void remove(const TCHAR *tag) { m_root->remove(tag); }
   void setRootDebugLevel(int level) { m_root->setWildcardDebugLevel(level); }
};

#endif
