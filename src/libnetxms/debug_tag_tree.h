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
   String *m_value;
   StringObjectMap<DebugTagTreeNode> *m_children;
   bool m_direct;
   bool m_asterisk;
   int m_directLvL;
   int m_asteriskLvL;

   DebugTagTreeNode(const TCHAR *value);

   int getDebugLvl(const StringList *tags, UINT32 pos);
   void add(const StringList *tags, UINT32 pos, UINT32 lvl);
   bool remove(const StringList *tags, UINT32 pos);

public:
   ~DebugTagTreeNode() { delete(m_value); delete(m_children); }
};

class DebugTagTree
{
private:
   DebugTagTreeNode *m_root;

public:
   DebugTagTree() { m_root = new DebugTagTreeNode(_T("")); }
   ~DebugTagTree() { delete(m_root); }

   void add(const TCHAR *tags, UINT32 lvl);
   void remove(const TCHAR *tags);
   int getDebugLvl(const TCHAR *tags);
};
