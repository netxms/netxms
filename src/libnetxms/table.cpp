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
	m_title = NULL;
   m_source = DS_INTERNAL;
   m_columns = new ObjectArray<TableColumnDefinition>(8, 8, true);
}

/**
 * Create table from NXCP message
 */
Table::Table(CSCPMessage *msg)
{
   m_columns = new ObjectArray<TableColumnDefinition>(8, 8, true);
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

   m_columns->clear();

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
	UINT32 dwId;

	m_nNumRows = msg->GetVariableLong(VID_TABLE_NUM_ROWS);
	m_nNumCols = msg->GetVariableLong(VID_TABLE_NUM_COLS);
	m_title = msg->GetVariableStr(VID_TABLE_TITLE);
   m_source = (int)msg->GetVariableShort(VID_DCI_SOURCE_TYPE);

	for(i = 0, dwId = VID_TABLE_COLUMN_INFO_BASE; i < m_nNumCols; i++, dwId += 10)
	{
      m_columns->add(new TableColumnDefinition(msg, dwId));
	}
   if (msg->IsVariableExist(VID_INSTANCE_COLUMN))
   {
      TCHAR name[MAX_COLUMN_NAME];
      msg->GetVariableStr(VID_INSTANCE_COLUMN, name, MAX_COLUMN_NAME);
      for(i = 0; i < m_columns->size(); i++)
      {
         if (!_tcsicmp(m_columns->get(i)->getName(), name))
         {
            m_columns->get(i)->setInstanceColumn(true);
            break;
         }
      }
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
	UINT32 id;

	msg.SetVariable(VID_TABLE_TITLE, CHECK_NULL_EX(m_title));
   msg.SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_source);

	if (offset == 0)
	{
		msg.SetVariable(VID_TABLE_NUM_ROWS, (UINT32)m_nNumRows);
		msg.SetVariable(VID_TABLE_NUM_COLS, (UINT32)m_nNumCols);

      for(i = 0, id = VID_TABLE_COLUMN_INFO_BASE; i < m_columns->size(); i++, id += 10)
         m_columns->get(i)->fillMessage(&msg, id);
	}
	msg.SetVariable(VID_TABLE_OFFSET, (UINT32)offset);

	int stopRow = (rowLimit == -1) ? m_nNumRows : min(m_nNumRows, offset + rowLimit);
	for(i = offset * m_nNumCols, row = offset, id = VID_TABLE_DATA_BASE; row < stopRow; row++)
	{
		for(col = 0; col < m_nNumCols; col++) 
		{
			TCHAR *tmp = m_ppData[i++];
			msg.SetVariable(id++, CHECK_NULL_EX(tmp));
		}
	}
	msg.SetVariable(VID_NUM_ROWS, (UINT32)(row - offset));

	if (row == m_nNumRows)
		msg.SetEndOfSequence();
	return row;
}

/**
 * Add new column
 */
int Table::addColumn(const TCHAR *name, INT32 dataType, bool isInstance, const TCHAR *displayName)
{
   m_columns->add(new TableColumnDefinition(name, displayName, dataType, isInstance));
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
   for(int i = 0; i < m_columns->size(); i++)
      if (!_tcsicmp(name, m_columns->get(i)->getName()))
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

/**
 * Set data at position
 */
void Table::setAt(int nRow, int nCol, const TCHAR *pszData)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return;

   safe_free(m_ppData[nRow * m_nNumCols + nCol]);
   m_ppData[nRow * m_nNumCols + nCol] = _tcsdup(pszData);
}

/**
 * Set pre-allocated data at position
 */
void Table::setPreallocatedAt(int nRow, int nCol, TCHAR *pszData)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return;

   safe_free(m_ppData[nRow * m_nNumCols + nCol]);
   m_ppData[nRow * m_nNumCols + nCol] = pszData;
}

/**
 * Set integer data at position
 */
void Table::setAt(int nRow, int nCol, INT32 nData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%d"), (int)nData);
   setAt(nRow, nCol, szBuffer);
}

/**
 * Set unsigned integer data at position
 */
void Table::setAt(int nRow, int nCol, UINT32 dwData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%u"), (unsigned int)dwData);
   setAt(nRow, nCol, szBuffer);
}

/**
 * Set 64 bit integer data at position
 */
void Table::setAt(int nRow, int nCol, INT64 nData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, INT64_FMT, nData);
   setAt(nRow, nCol, szBuffer);
}

/**
 * Set unsigned 64 bit integer data at position
 */
void Table::setAt(int nRow, int nCol, UINT64 qwData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, UINT64_FMT, qwData);
   setAt(nRow, nCol, szBuffer);
}

/**
 * Set floating point data at position
 */
void Table::setAt(int nRow, int nCol, double dData)
{
   TCHAR szBuffer[32];

   _sntprintf(szBuffer, 32, _T("%f"), dData);
   setAt(nRow, nCol, szBuffer);
}

/**
 * Get data from position
 */
const TCHAR *Table::getAsString(int nRow, int nCol)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return NULL;

   return m_ppData[nRow * m_nNumCols + nCol];
}

INT32 Table::getAsInt(int nRow, int nCol)
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstol(pszVal, NULL, 0) : 0;
}

UINT32 Table::getAsUInt(int nRow, int nCol)
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

UINT64 Table::getAsUInt64(int nRow, int nCol)
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
void Table::merge(Table *t)
{
   if (m_nNumCols != t->m_nNumCols)
      return;

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

/**
 * Create new table column definition
 */
TableColumnDefinition::TableColumnDefinition(const TCHAR *name, const TCHAR *displayName, INT32 dataType, bool isInstance)
{
   m_name = _tcsdup(CHECK_NULL(name));
   m_displayName = (displayName != NULL) ? _tcsdup(displayName) : _tcsdup(m_name);
   m_dataType = dataType;
   m_instanceColumn = isInstance;
}

/**
 * Create copy of existing table column definition
 */
TableColumnDefinition::TableColumnDefinition(TableColumnDefinition *src)
{
   m_name = _tcsdup(src->m_name);
   m_displayName = _tcsdup(src->m_displayName);
   m_dataType = src->m_dataType;
   m_instanceColumn = src->m_instanceColumn;
}

/**
 * Create table column definition from NXCP message
 */
TableColumnDefinition::TableColumnDefinition(CSCPMessage *msg, UINT32 baseId)
{
   m_name = msg->GetVariableStr(baseId);
   if (m_name == NULL)
      m_name = _tcsdup(_T("(null)"));
   m_dataType = msg->GetVariableLong(baseId + 1);
   m_displayName = msg->GetVariableStr(baseId + 2);
   if (m_displayName == NULL)
      m_displayName = _tcsdup(m_name);
   m_instanceColumn = msg->GetVariableShort(baseId + 3) ? true : false;
}

/**
 * Destructor for table column definition
 */
TableColumnDefinition::~TableColumnDefinition()
{
   free(m_name);
   free(m_displayName);
}

/**
 * Fill message with table column definition data
 */
void TableColumnDefinition::fillMessage(CSCPMessage *msg, UINT32 baseId)
{
   msg->SetVariable(baseId, m_name);
   msg->SetVariable(baseId + 1, (UINT32)m_dataType);
   msg->SetVariable(baseId + 2, m_displayName);
   msg->SetVariable(baseId + 3, (WORD)(m_instanceColumn ? 1 : 0));
}
