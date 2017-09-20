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
#include <nms_util.h>

class DebugTagTreeNode
{
   friend class DebugTagTree;

private:
   TCHAR *m_value;
   StringObjectMap<DebugTagTreeNode> *m_children;
   bool m_direct;
   bool m_asterisk;
   UINT32 m_directLvL;
   UINT32 m_asteriskLvL;

   DebugTagTreeNode();
   DebugTagTreeNode(const TCHAR *value, size_t len);

   int getDebugLvl(const TCHAR *tags);
   const TCHAR *getValue() const { return m_value; }
   void add(const TCHAR *tags, UINT32 lvl);
   bool remove(const TCHAR *tags);

public:
   ~DebugTagTreeNode() { free(m_value); delete(m_children); }
};

class DebugTagTree
{
private:
   DebugTagTreeNode *m_root;
   VolatileCounter m_readerCount;

public:
   DebugTagTree() { m_root = new DebugTagTreeNode(); m_readerCount = 0; }
   ~DebugTagTree() { delete(m_root); }

   void add(const TCHAR *tags, UINT32 lvl);
   void remove(const TCHAR *tags);
   UINT32 getDebugLvl(const TCHAR *tags);

   void setRootDebugLvl(UINT32 lvl);
   UINT32 getRootDebugLvl();

   INT32 getReaderCount() { return (INT32)m_readerCount; }
};
