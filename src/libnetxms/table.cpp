/* $Id: table.cpp,v 1.1 2006-11-03 08:58:58 victor Exp $ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: table.cpp
**
**/

#include "libnetxms.h"


//
// Table constructor
//

Table::Table()
{
   m_nNumRows = 0;
   m_nNumCols = 0;
   m_ppData = NULL;
   m_ppColNames = NULL;
}


//
// Table destructor
//

Table::~Table()
{
   int i;

   for(i = 0; i < m_nNumCols; i++)
      safe_free(m_ppColNames[i]);
   safe_free(m_ppColNames);

   for(i = 0; i < m_nNumRows * m_nNumCols; i++)
      safe_free(m_ppData[i]);
   safe_free(m_ppData);
}


//
// Add new column
//

void Table::AddColumn(TCHAR *pszName)
{
   m_ppColNames = (TCHAR **)realloc(m_ppColNames, sizeof(TCHAR *) * (m_nNumCols + 1));
   m_ppColNames[m_nNumCols] = _tcsdup(pszName);

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
}


//
// Add new row
//

void Table::AddRow(void)
{
   if (m_nNumCols > 0)
   {
      m_ppData = (TCHAR **)realloc(m_ppData, sizeof(TCHAR *) * (m_nNumRows + 1) * m_nNumCols);
      memset(&m_ppData[m_nNumRows * m_nNumCols], 0, sizeof(TCHAR *) * m_nNumCols);
   }
   m_nNumRows++;
}


//
// Set data at position
//

void Table::SetAt(int nRow, int nCol, TCHAR *pszData)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return;

   safe_free(m_ppData[nRow * m_nNumCols + nCol]);
   m_ppData[nRow * m_nNumCols + nCol] = _tcsdup(pszData);
}

void Table::SetAt(int nRow, int nCol, LONG nData)
{
   TCHAR szBuffer[32];

   _stprintf(szBuffer, _T("%d"), nData);
   SetAt(nRow, nCol, szBuffer);
}

void Table::SetAt(int nRow, int nCol, DWORD dwData)
{
   TCHAR szBuffer[32];

   _stprintf(szBuffer, _T("%u"), dwData);
   SetAt(nRow, nCol, szBuffer);
}

void Table::SetAt(int nRow, int nCol, INT64 nData)
{
   TCHAR szBuffer[32];

   _stprintf(szBuffer, INT64_FMT, nData);
   SetAt(nRow, nCol, szBuffer);
}

void Table::SetAt(int nRow, int nCol, QWORD qwData)
{
   TCHAR szBuffer[32];

   _stprintf(szBuffer, UINT64_FMT, qwData);
   SetAt(nRow, nCol, szBuffer);
}

void Table::SetAt(int nRow, int nCol, double dData)
{
   TCHAR szBuffer[32];

   _stprintf(szBuffer, _T("%f"), dData);
   SetAt(nRow, nCol, szBuffer);
}


//
// Get data from position
//

TCHAR *Table::GetAsString(int nRow, int nCol)
{
   if ((nRow < 0) || (nRow >= m_nNumRows) ||
       (nCol < 0) || (nCol >= m_nNumCols))
      return NULL;

   return m_ppData[nRow * m_nNumCols + nCol];
}

LONG Table::GetAsInt(int nRow, int nCol)
{
   TCHAR *pszVal;

   pszVal = GetAsString(nRow, nCol);
   return pszVal != NULL ? _tcstol(pszVal, NULL, 0) : 0;
}

DWORD Table::GetAsUInt(int nRow, int nCol)
{
   TCHAR *pszVal;

   pszVal = GetAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoul(pszVal, NULL, 0) : 0;
}

INT64 Table::GetAsInt64(int nRow, int nCol)
{
   TCHAR *pszVal;

   pszVal = GetAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoll(pszVal, NULL, 0) : 0;
}

QWORD Table::GetAsUInt64(int nRow, int nCol)
{
   TCHAR *pszVal;

   pszVal = GetAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoull(pszVal, NULL, 0) : 0;
}

double Table::GetAsDouble(int nRow, int nCol)
{
   TCHAR *pszVal;

   pszVal = GetAsString(nRow, nCol);
   return pszVal != NULL ? _tcstod(pszVal, NULL) : 0;
}
