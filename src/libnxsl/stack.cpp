/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: stack.cpp
**
**/

#include "libnxsl.h"

/**
 * Constructor
 */
NXSL_Stack::NXSL_Stack()
{
   m_nStackSize = 128;
   m_nStackPos = 0;
   m_ppData = (void **)malloc(sizeof(void *) * m_nStackSize);
}

/**
 * Destructor
 */
NXSL_Stack::~NXSL_Stack()
{
   free(m_ppData);
}

/**
 * Push value to stack
 */
void NXSL_Stack::push(void *pData)
{
   if (m_nStackPos >= m_nStackSize)
   {
      m_nStackSize += 64;
      m_ppData = (void **)realloc(m_ppData, sizeof(void *) * m_nStackSize);
   }
   m_ppData[m_nStackPos++] = pData;
}

/**
 * Pop value from stack
 */
void *NXSL_Stack::pop()
{
   if (m_nStackPos > 0)
      return m_ppData[--m_nStackPos];
   return NULL;
}

/**
 * Peek (get without removing) value from stack
 */
void *NXSL_Stack::peek()
{
   if (m_nStackPos > 0)
      return m_ppData[m_nStackPos - 1];
   return NULL;
}

/**
 * Peek (get without removing) value from stack at given offset from top
 */
void *NXSL_Stack::peekAt(int offset)
{
   if ((offset > 0) && (m_nStackPos > offset - 1))
      return m_ppData[m_nStackPos - offset];
   return NULL;
}
