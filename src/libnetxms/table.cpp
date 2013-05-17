/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: table.cpp
**
**/

#include "libnetxms.h"

/**
 * Create empty table
 */
Table::Table()
{
   m_nNumRows = 0;
   m_nNumCols = 0;
   m_ppData = NULL;
   m_ppColNames = NULL;
	m_colFormats = NULL;
	m_title = NULL;
   m_source = DS_INTERNAL;
}

/**
 * Create table from NXCP message
 */
Table::Table(CSCPMessage *msg)
{
	createFromMessage(msg);
}

/**
 * Table destructor
 */
Table::~Table()
{
	destroy();
}

/**
 * Destroy table data
 */
void Table::destroy()
{
   int i;

   for(i = 0; i < m_nNumCols; i++)
      safe_free(m_ppColNames[i]);
   safe_free(m_ppColNames);

	safe_free(m_colFormats);

   for(i = 0; i < m_nNumRows * m_nNumCols; i++)
      safe_free(m_ppData[i]);
   safe_free(m_ppData);

	safe_free(m_title);
}

/**
 * Create table from NXCP message
 */
void Table::createFromMessage(CSCPMessage *msg)
{
	int i;
	DWORD dwId;

	m_nNumRows = msg->GetVariableLong(VID_TABLE_NUM_ROWS);
	m_nNumCols = msg->GetVariableLong(VID_TABLE_NUM_COLS);
	m_title = msg->GetVariableStr(VID_TABLE_TITLE);
   m_source = (int)msg->GetVariableShort(VID_DCI_SOURCE_TYPE);

	m_ppColNames = (TCHAR **)malloc(sizeof(TCHAR *) * m_nNumCols);
	m_colFormats = (LONG *)malloc(sizeof(LONG) * m_nNumCols);
	for(i = 0, dwId = VID_TABLE_COLUMN_INFO_BASE; i < m_nNumCols; i++, dwId += 8)
	{
		m_ppColNames[i] = msg->GetVariableStr(dwId++);
		m_colFormats[i] = (LONG)msg->GetVariableLong(dwId++);
	}

	m_ppData = (TCHAR **)malloc(sizeof(TCHAR *) * m_nNumCols * m_nNumRows);
	for(i = 0, dwId = VID_TABLE_DATA_BASE; i < m_nNumCols * m_nNumRows; i++)
		m_ppData[i] = msg->GetVariableStr(dwId++);
}

/**
 * Update table from NXCP message
 */
void Table::updateFromMessage(CSCPMessage *msg)
{
	destroy();
	createFromMessage(msg);
}

/**
 * Fill NXCP message with table data
 */
int Table::fillMessage(CSCPMessage &msg, int offset, int rowLimit)
{
	int i, row, col;
	DWORD id;

	msg.SetVariable(VID_TABLE_TITLE, CHECK_NULL_EX(m_title));
   msg.SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_source);

	if (offset == 0)
	{
		msg.SetVariable(VID_TABLE_NUM_ROWS, (DWORD)m_nNumRows);
		msg.SetVariable(VID_TABLE_NUM_COLS, (DWORD)m_nNumCols);

		for(i = 0, id = VID_TABLE_COLUMN_INFO_BASE; i < m_nNumCols; i++, id += 8)
		{
			msg.SetVariable(id++, CHECK_NULL_EX(m_ppColNames[i]));
			msg.SetVariable(id++, (DWORD)m_colFormats[i]);
		}
	}
	msg.SetVariable(VID_TABLE_OFFSET, (DWORD)offset);

	int stopRow = (rowLimit == -1) ? m_nNumRows : min(m_nNumRows, offset + rowLimit);
	for(i = offset * m_nNumCols, row = offset, id = VID_TABLE_DATA_BASE; row < stopRow; row++)
	{
		for(col = 0; col < m_nNumCols; col++) 
		{
			TCHAR *tmp = m_ppData[i++];
			msg.SetVariable(id++, CHECK_NULL_EX(tmp));
		}
	}
	msg.SetVariable(VID_NUM_ROWS, (DWORD)(row - offset));

	if (row == m_nNumRows)
		msg.SetEndOfSequence();
	return row;
}

/**
 * Add new column
 */
int Table::addColumn(const TCHAR *name, LONG format)
{
   m_ppColNames = (TCHAR **)realloc(m_ppColNames, sizeof(TCHAR *) * (m_nNumCols + 1));
   m_ppColNames[m_nNumCols] = _tcsdup(name);

	m_colFormats = (LONG *)realloc(m_colFormats, sizeof(LONG) * (m_nNumCols + 1));
	m_colFormats[m_nNumCols] = format;

   if (m_nNumRows > 0)
   {
      TCHAR **ppNewData;
      int i, nPosOld, nPosNew;

      ppNewData = (TCHAR **)malloc(sizeof(TCHAR *) * m_nNumRows * (m_nNumCols + 1));
      for(i = 0, nPosOld = 0, nPosNew = 0; i < m_nNumRows; i++)
      {
         memcpy(&ppNewData[nPosNew], &m_ppData[nPosOld], sizeof(TCHAR *) * m_nNumCols);
         ppNewData[nPosNew + m_nNumCols] = NULL;
         nPosOld += m_nNumCols;
         nPosNew += m_nNumCols + 1;
      }
      safe_free(m_ppData);
      m_ppData = ppNewData;
   }

   m_nNumCols++;
	return m_nNumCols - 1;
}

/**
 * Get column index by name
 *
 * @param name column name
 * @return column index or -1 if there are no such column
 */
int Table::getColumnIndex(const TCHAR *name)
{
   for(int i = 0; i < m_nNumCols; i++)
      if (!_tcsicmp(name, m_ppColNames[i]))
         return i;
   return -1;
}

/**
 * Add new row
 */
int Table::addRow()
{
   if (m_nNumCols > 0)
   {
      m_ppData = (TCHAR **)realloc(m_ppData, sizeof(TCHAR *) * (m_nNumRows + 1) * m_nNumCols);
      memset(&m_ppData[m_nNumRows * m_nNumCols], 0, sizeof(TCHAR *) * m_nNumCols);
   }
   m_nNumRows++;
	return m_nNumRows - 1;
}


//
// Set data at position
//

void Table::setAt(int nRow, int nCol, const TCHAR *pszData)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return;

   safe_free(m_ppData[nRow * m_nNumCols + nCol]);
   m_ppData[nRow * m_nNumCols + nCol] = _tcsdup(pszData);
}

void Table::setPreallocatedAt(int nRow, int nCol, TCHAR *pszData)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return;

   safe_free(m_ppData[nRow * m_nNumCols + nCol]);
   m_ppData[nRow * m_nNumCols + nCol] = pszData;
}

void Table::setAt(int nRow, int nCol, LONG nData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%d"), (int)nData);
   setAt(nRow, nCol, szBuffer);
}

void Table::setAt(int nRow, int nCol, DWORD dwData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%u"), (unsigned int)dwData);
   setAt(nRow, nCol, szBuffer);
}

void Table::setAt(int nRow, int nCol, INT64 nData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, INT64_FMT, nData);
   setAt(nRow, nCol, szBuffer);
}

void Table::setAt(int nRow, int nCol, QWORD qwData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, UINT64_FMT, qwData);
   setAt(nRow, nCol, szBuffer);
}

void Table::setAt(int nRow, int nCol, double dData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%f"), dData);
   setAt(nRow, nCol, szBuffer);
}


//
// Get data from position
//

const TCHAR *Table::getAsString(int nRow, int nCol)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return NULL;

   return m_ppData[nRow * m_nNumCols + nCol];
}

LONG Table::getAsInt(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstol(pszVal, NULL, 0) : 0;
}

DWORD Table::getAsUInt(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoul(pszVal, NULL, 0) : 0;
}

INT64 Table::getAsInt64(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoll(pszVal, NULL, 0) : 0;
}

QWORD Table::getAsUInt64(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoull(pszVal, NULL, 0) : 0;
}

double Table::getAsDouble(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstod(pszVal, NULL) : 0;
}

/**
 * Merge with another table. If instance column given, values from rows
 * with same values in instance column will be summarized.
 * Identical table format assumed.
 *
 * @param t table to merge into this table
 * @param instanceColumn name of instance column, may be NULL
 */
void Table::merge(Table *t, const TCHAR *instanceColumn)
{
   if (m_nNumCols != t->m_nNumCols)
      return;

   int instColIdx = (instanceColumn != NULL) ? getColumnIndex(instanceColumn) : -1;
   if (instColIdx != -1)
   {
      for(int i = 0; i < t->m_nNumRows; i++)
      {
         const TCHAR *instance = t->getAsString(i, instColIdx);
         
      }
   }
   else
   {
      m_ppData = (TCHAR **)realloc(m_ppData, sizeof(TCHAR *) * (m_nNumRows + t->m_nNumRows) * m_nNumCols);
      int dpos = m_nNumRows * m_nNumCols;
      int spos = 0;
      for(int i = 0; i < t->m_nNumRows; i++)
      {
         for(int j = 0; j < m_nNumCols; j++)
         {
            const TCHAR *value = t->m_ppData[spos++];
            m_ppData[dpos++] = (value != NULL) ? _tcsdup(value) : NULL;
         }
      }
   }
}
