/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
#include <nxcpapi.h>
#include <expat.h>
#include <zlib.h>

#define DEFAULT_OBJECT_ID  (0)
#define DEFAULT_STATUS     (-1)

/**
 * Create empty table row
 */
TableRow::TableRow(int columnCount)
{
   m_cells = new ObjectArray<TableCell>(columnCount, 8, true);
   for(int i = 0; i < columnCount; i++)
      m_cells->add(new TableCell());
   m_objectId = 0;
   m_baseRow = -1;
}

/**
 * Table row copy constructor
 */
TableRow::TableRow(TableRow *src)
{
   m_cells = new ObjectArray<TableCell>(src->m_cells->size(), 8, true);
   for(int i = 0; i < src->m_cells->size(); i++)
      m_cells->add(new TableCell(src->m_cells->get(i)));
   m_objectId = src->m_objectId;
   m_baseRow = src->m_baseRow;
}

/**
 * Create empty table
 */
Table::Table() : RefCountObject()
{
   m_data = new ObjectArray<TableRow>(32, 32, true);
	m_title = NULL;
   m_source = DS_INTERNAL;
   m_columns = new ObjectArray<TableColumnDefinition>(8, 8, true);
   m_extendedFormat = false;
}

/**
 * Create table from NXCP message
 */
Table::Table(NXCPMessage *msg) : RefCountObject()
{
   m_columns = new ObjectArray<TableColumnDefinition>(8, 8, true);
	createFromMessage(msg);
}

/**
 * Copy constructor
 */
Table::Table(Table *src) : RefCountObject()
{
   m_extendedFormat = src->m_extendedFormat;
   m_data = new ObjectArray<TableRow>(src->m_data->size(), 32, true);
	int i;
   for(i = 0; i < src->m_data->size(); i++)
      m_data->add(new TableRow(src->m_data->get(i)));
   m_title = (src->m_title != NULL) ? _tcsdup(src->m_title) : NULL;
   m_source = src->m_source;
   m_columns = new ObjectArray<TableColumnDefinition>(src->m_columns->size(), 8, true);
	for(i = 0; i < src->m_columns->size(); i++)
      m_columns->add(new TableColumnDefinition(src->m_columns->get(i)));
}

/**
 * Table destructor
 */
Table::~Table()
{
   destroy();
   delete m_columns;
   delete m_data;
}

/**
 * Destroy table data
 */
void Table::destroy()
{
   m_columns->clear();
   m_data->clear();
   safe_free(m_title);
}

/**
 * XML parser state for creating LogParser object from XML
 */
#define XML_STATE_INIT        -1
#define XML_STATE_END         -2
#define XML_STATE_ERROR       -255
#define XML_STATE_TABLE       0
#define XML_STATE_COLUMNS     1
#define XML_STATE_COLUMN      2
#define XML_STATE_DATA        3
#define XML_STATE_ROW         4
#define XML_STATE_CELL        5

/**
 * State information for XML parser
 */
typedef struct
{
   Table *table;
	int state;
	String *buffer;
	int column;
} XML_PARSER_STATE;

/**
 * Element start handler for XML parser
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (!strcmp(name, "table"))
	{
      if (ps->state == XML_STATE_INIT)
      {
         ps->table->setExtendedFormat(XMLGetAttrBoolean(attrs, "extendedFormat", false));
         ps->table->setSource(XMLGetAttrInt(attrs, "source", 0));
         const char *title = XMLGetAttr(attrs, "name");
		   if (title != NULL)
		   {
#ifdef UNICODE
			   WCHAR *wtitle = WideStringFromUTF8String(title);
            ps->table->setTitle(wtitle);
			   free(wtitle);
#else
            ps->table->setTitle(title);
#endif
		   }
		   ps->state = XML_STATE_TABLE;
      }
      else
      {
		   ps->state = XML_STATE_ERROR;
      }
	}
	else if (!strcmp(name, "columns"))
	{
      ps->state = (ps->state == XML_STATE_TABLE) ? XML_STATE_COLUMNS : XML_STATE_ERROR;
	}
	else if (!strcmp(name, "column"))
	{
      if (ps->state == XML_STATE_COLUMNS)
      {
#ifdef UNICODE
         wchar_t *name = WideStringFromUTF8String(CHECK_NULL_A(XMLGetAttr(attrs, "name")));
         const char *tmp = XMLGetAttr(attrs, "displayName");
         wchar_t *displayName = (tmp != NULL) ? WideStringFromUTF8String(tmp) : NULL;
#else
         const char *name = CHECK_NULL_A(XMLGetAttr(attrs, "name"));
         const char *displayName = XMLGetAttr(attrs, "displayName");
#endif
         ps->table->addColumn(name, XMLGetAttrInt(attrs, "dataType", 0), displayName, XMLGetAttrBoolean(attrs, "isInstance", false));
		   ps->state = XML_STATE_COLUMN;
#ifdef UNICODE
         safe_free(name);
         safe_free(displayName);
#endif
      }
      else
      {
		   ps->state = XML_STATE_ERROR;
      }
	}
	else if (!strcmp(name, "data"))
	{
      ps->state = (ps->state == XML_STATE_TABLE) ? XML_STATE_DATA : XML_STATE_ERROR;
	}
	else if (!strcmp(name, "tr"))
	{
      if (ps->state == XML_STATE_DATA)
      {
         ps->table->addRow();
         ps->table->setObjectId(XMLGetAttrInt(attrs, "objectId", DEFAULT_OBJECT_ID));
         ps->table->setBaseRow(XMLGetAttrInt(attrs, "baseRow", -1));
         ps->column = 0;
		   ps->state = XML_STATE_ROW;
      }
      else
      {
		   ps->state = XML_STATE_ERROR;
      }
	}
	else if (!strcmp(name, "td"))
	{
      if (ps->state == XML_STATE_ROW)
      {
         ps->table->setStatus(ps->column, XMLGetAttrInt(attrs, "status", DEFAULT_STATUS));
         ps->state = XML_STATE_CELL;
         ps->buffer->clear();
      }
      else
      {
		   ps->state = XML_STATE_ERROR;
      }
	}
	else
	{
		ps->state = XML_STATE_ERROR;
	}
}

/**
 * Element end handler for XML parser
 */
static void EndElement(void *userData, const char *name)
{
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;
   if (ps->state == XML_STATE_ERROR)
      return;

   if (!strcmp(name, "td"))
	{
      ps->table->set(ps->column, ps->buffer->getBuffer());
      ps->column++;
		ps->state = XML_STATE_ROW;
	}
	else if (!strcmp(name, "tr"))
	{
      ps->column = -1;
      ps->state = XML_STATE_DATA;
	}
	else if (!strcmp(name, "column"))
	{
      ps->state = XML_STATE_COLUMNS;
	}
	else if (!strcmp(name, "columns") || !strcmp(name, "data"))
	{
      ps->state = XML_STATE_TABLE;
	}
}

/**
 * Data handler for XML parser
 */
static void CharData(void *userData, const XML_Char *s, int len)
{
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

   if (ps->state == XML_STATE_CELL)
   {
      ps->buffer->appendMBString(s, len, CP_UTF8);
   }
}

/**
 * Parse XML document with table data
 */
bool Table::parseXML(const char *xml)
{
   XML_PARSER_STATE state;

   XML_Parser parser = XML_ParserCreate(NULL);
   XML_SetUserData(parser, &state);
   XML_SetElementHandler(parser, StartElement, EndElement);
   XML_SetCharacterDataHandler(parser, CharData);

   state.table = this;
   state.state = XML_STATE_INIT;
   state.column = -1;
   state.buffer = new String();

   bool success = (XML_Parse(parser, xml, (int)strlen(xml), TRUE) != XML_STATUS_ERROR);
   if (success)
      success = (state.state != XML_STATE_ERROR);
   XML_ParserFree(parser);
   delete state.buffer;
   return success;
}

/**
 * Create table from XML document
 */
Table *Table::createFromXML(const char *xml)
{
   Table *table = new Table();
   if (table->parseXML(xml))
   {
      return table;
   }
   delete table;
   return NULL;
}

/**
 * Create table from packed XML document
 */
Table *Table::createFromPackedXML(const char *packedXml)
{
   char *compressedXml = NULL;
   size_t compressedSize = 0;
   base64_decode_alloc(packedXml, strlen(packedXml), &compressedXml, &compressedSize);
   if (compressedXml == NULL)
      return NULL;

   size_t xmlSize = (size_t)ntohl(*((UINT32 *)compressedXml));
   char *xml = (char *)malloc(xmlSize + 1);
   uLongf uncompSize = (uLongf)xmlSize;
   if (uncompress((BYTE *)xml, &uncompSize, (BYTE *)&compressedXml[4], (uLong)compressedSize - 4) != Z_OK)
   {
      free(xml);
      free(compressedXml);
      return NULL;
   }
   xml[xmlSize] = 0;
   free(compressedXml);

   Table *table = new Table();
   if (table->parseXML(xml))
   {
      free(xml);
      return table;
   }
   free(xml);
   delete table;
   return NULL;
}

/**
 * Create XML document from table
 */
TCHAR *Table::createXML() const
{
   String xml;
   xml.appendFormattedString(_T("<table extendedFormat=\"%s\" source=\"%d\"  name=\"%s\">\r\n"), m_extendedFormat ? _T("true") : _T("false"), m_source,
                              (const TCHAR *)EscapeStringForXML2(m_title, -1));
   xml.append(_T("<columns>\r\n"));
   int i;
   for(i = 0; i < m_columns->size(); i++)
      xml.appendFormattedString(_T("<column name=\"%s\" displayName=\"%s\" isInstance=\"%s\" dataType=\"%d\"/>\r\n"),
                  (const TCHAR *)EscapeStringForXML2(m_columns->get(i)->getName(), -1),
                  (const TCHAR *)EscapeStringForXML2(m_columns->get(i)->getDisplayName(), -1),
                  m_columns->get(i)->isInstanceColumn()? _T("true") : _T("false"), m_columns->get(i)->getDataType());
   xml.append(_T("</columns>\r\n"));
   xml.append(_T("<data>\r\n"));
   for(i = 0; i < m_data->size(); i++)
   {
      UINT32 objectId = m_data->get(i)->getObjectId();
      int baseRow = m_data->get(i)->getBaseRow();
      if (objectId != DEFAULT_OBJECT_ID)
      {
         if (baseRow != -1)
            xml.appendFormattedString(_T("<tr objectId=\"%u\" baseRow=\"%d\">\r\n"), objectId, baseRow);
         else
            xml.appendFormattedString(_T("<tr objectId=\"%u\">\r\n"), objectId);
      }
      else
      {
         if (baseRow != -1)
            xml.appendFormattedString(_T("<tr baseRow=\"%d\">\r\n"), baseRow);
         else
            xml.append(_T("<tr>\r\n"));
      }
      for(int j = 0; j < m_columns->size(); j++)
      {
         int status = m_data->get(i)->getStatus(j);
         if (status != DEFAULT_STATUS)
         {
            xml.append(_T("<td status=\""));
            xml.append(status);
            xml.append(_T("\">"));
         }
         else
         {
            xml.append(_T("<td>"));
         }
         xml.append((const TCHAR *)EscapeStringForXML2(m_data->get(i)->getValue(j), -1));
         xml.append(_T("</td>\r\n"));
      }
      xml.append(_T("</tr>\r\n"));
   }
   xml.append(_T("</data>\r\n"));
   xml.append(_T("</table>"));
   return _tcsdup(xml);
}

/**
 * Create packed XML document
 */
char *Table::createPackedXML() const
{
   TCHAR *xml = createXML();
   if (xml == NULL)
      return NULL;
   char *utf8xml = UTF8StringFromTString(xml);
   free(xml);
   size_t len = strlen(utf8xml);
   uLongf buflen = compressBound((uLong)len);
   BYTE *buffer = (BYTE *)malloc(buflen + 4);
   if (compress(&buffer[4], &buflen, (BYTE *)utf8xml, (uLong)len) != Z_OK)
   {
      free(utf8xml);
      free(buffer);
      return NULL;
   }
   free(utf8xml);
   char *encodedBuffer = NULL;
   *((UINT32 *)buffer) = htonl((UINT32)len);
   base64_encode_alloc((char *)buffer, buflen + 4, &encodedBuffer);
   free(buffer);
   return encodedBuffer;
}

/**
 * Create table from NXCP message
 */
void Table::createFromMessage(NXCPMessage *msg)
{
	int i;
	UINT32 dwId;

	int rows = msg->getFieldAsUInt32(VID_TABLE_NUM_ROWS);
	int columns = msg->getFieldAsUInt32(VID_TABLE_NUM_COLS);
	m_title = msg->getFieldAsString(VID_TABLE_TITLE);
   m_source = msg->getFieldAsInt16(VID_DCI_SOURCE_TYPE);
   m_extendedFormat = msg->getFieldAsBoolean(VID_TABLE_EXTENDED_FORMAT);

	for(i = 0, dwId = VID_TABLE_COLUMN_INFO_BASE; i < columns; i++, dwId += 10)
	{
      m_columns->add(new TableColumnDefinition(msg, dwId));
	}
   if (msg->isFieldExist(VID_INSTANCE_COLUMN))
   {
      TCHAR name[MAX_COLUMN_NAME];
      msg->getFieldAsString(VID_INSTANCE_COLUMN, name, MAX_COLUMN_NAME);
      for(i = 0; i < m_columns->size(); i++)
      {
         if (!_tcsicmp(m_columns->get(i)->getName(), name))
         {
            m_columns->get(i)->setInstanceColumn(true);
            break;
         }
      }
   }

	m_data = new ObjectArray<TableRow>(rows, 32, true);
	for(i = 0, dwId = VID_TABLE_DATA_BASE; i < rows; i++)
   {
      TableRow *row = new TableRow(columns);
		m_data->add(row);
      if (m_extendedFormat)
      {
         row->setObjectId(msg->getFieldAsUInt32(dwId++));
         if (msg->isFieldExist(dwId))
            row->setBaseRow(msg->getFieldAsInt32(dwId));
         dwId += 9;
      }
      for(int j = 0; j < columns; j++)
      {
         TCHAR *value = msg->getFieldAsString(dwId++);
         if (m_extendedFormat)
         {
            int status = msg->getFieldAsInt16(dwId++);
            UINT32 objectId = msg->getFieldAsUInt32(dwId++);
            row->setPreallocated(j, value, status, objectId);
            dwId += 7;
         }
         else
         {
            row->setPreallocated(j, value, -1, 0);
         }
      }
   }
}

/**
 * Update table from NXCP message
 */
void Table::updateFromMessage(NXCPMessage *msg)
{
	destroy();
   delete m_data; // will be re-created by createFromMessage
	createFromMessage(msg);
}

/**
 * Fill NXCP message with table data
 */
int Table::fillMessage(NXCPMessage &msg, int offset, int rowLimit)
{
	UINT32 id;

	msg.setField(VID_TABLE_TITLE, CHECK_NULL_EX(m_title));
   msg.setField(VID_DCI_SOURCE_TYPE, (UINT16)m_source);
   msg.setField(VID_TABLE_EXTENDED_FORMAT, (UINT16)(m_extendedFormat ? 1 : 0));

	if (offset == 0)
	{
		msg.setField(VID_TABLE_NUM_ROWS, (UINT32)m_data->size());
		msg.setField(VID_TABLE_NUM_COLS, (UINT32)m_columns->size());

      id = VID_TABLE_COLUMN_INFO_BASE;
      for(int i = 0; i < m_columns->size(); i++, id += 10)
         m_columns->get(i)->fillMessage(&msg, id);
	}
	msg.setField(VID_TABLE_OFFSET, (UINT32)offset);

	int stopRow = (rowLimit == -1) ? m_data->size() : std::min(m_data->size(), offset + rowLimit);
   id = VID_TABLE_DATA_BASE;
	for(int row = offset; row < stopRow; row++)
	{
      TableRow *r = m_data->get(row);
      if (m_extendedFormat)
      {
			msg.setField(id++, r->getObjectId());
         msg.setField(id++, r->getBaseRow());
         id += 8;
      }
		for(int col = 0; col < m_columns->size(); col++)
		{
			const TCHAR *tmp = r->getValue(col);
			msg.setField(id++, CHECK_NULL_EX(tmp));
         if (m_extendedFormat)
         {
            msg.setField(id++, (UINT16)r->getStatus(col));
            msg.setField(id++, r->getCellObjectId(col));
            id += 7;
         }
		}
	}
	msg.setField(VID_NUM_ROWS, (UINT32)(stopRow - offset));

	if (stopRow == m_data->size())
		msg.setEndOfSequence();
	return stopRow;
}

/**
 * Add new column
 */
int Table::addColumn(const TCHAR *name, INT32 dataType, const TCHAR *displayName, bool isInstance)
{
   m_columns->add(new TableColumnDefinition(name, displayName, dataType, isInstance));
   for(int i = 0; i < m_data->size(); i++)
      m_data->get(i)->addColumn();
	return m_columns->size() - 1;
}

/**
 * Add new column
 */
int Table::addColumn(const TableColumnDefinition *d)
{
   m_columns->add(new TableColumnDefinition(d));
   for(int i = 0; i < m_data->size(); i++)
      m_data->get(i)->addColumn();
   return m_columns->size() - 1;
}

/**
 * Get column index by name
 *
 * @param name column name
 * @return column index or -1 if there are no such column
 */
int Table::getColumnIndex(const TCHAR *name) const
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
   m_data->add(new TableRow(m_columns->size()));
	return m_data->size() - 1;
}

/**
 * Delete row
 */
void Table::deleteRow(int row)
{
   m_data->remove(row);
}

/**
 * Delete column
 */
void Table::deleteColumn(int col)
{
   if ((col < 0) || (col >= m_columns->size()))
      return;

   m_columns->remove(col);
   for(int i = 0; i < m_data->size(); i++)
      m_data->get(i)->deleteColumn(col);
}

/**
 * Set data at position
 */
void Table::setAt(int nRow, int nCol, const TCHAR *pszData)
{
   TableRow *r = m_data->get(nRow);
   if (r != NULL)
   {
      r->setValue(nCol, pszData);
   }
}

/**
 * Set pre-allocated data at position
 */
void Table::setPreallocatedAt(int nRow, int nCol, TCHAR *data)
{
   TableRow *r = m_data->get(nRow);
   if (r != NULL)
   {
      r->setPreallocatedValue(nCol, data);
   }
   else
   {
      free(data);
   }
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
const TCHAR *Table::getAsString(int nRow, int nCol, const TCHAR *defaultValue) const
{
   const TableRow *r = m_data->get(nRow);
   if (r == NULL)
      return defaultValue;
   const TCHAR *v = r->getValue(nCol);
   return (v != NULL) ? v : defaultValue;
}

INT32 Table::getAsInt(int nRow, int nCol) const
{
   const TCHAR *pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstol(pszVal, NULL, 0) : 0;
}

UINT32 Table::getAsUInt(int nRow, int nCol) const
{
   const TCHAR *pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoul(pszVal, NULL, 0) : 0;
}

INT64 Table::getAsInt64(int nRow, int nCol) const
{
   const TCHAR *pszVal;

   pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoll(pszVal, NULL, 0) : 0;
}

UINT64 Table::getAsUInt64(int nRow, int nCol) const
{
   const TCHAR *pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstoull(pszVal, NULL, 0) : 0;
}

double Table::getAsDouble(int nRow, int nCol) const
{
   const TCHAR *pszVal = getAsString(nRow, nCol);
   return pszVal != NULL ? _tcstod(pszVal, NULL) : 0;
}

/**
 * Set status of given cell
 */
void Table::setStatusAt(int row, int col, int status)
{
   TableRow *r = m_data->get(row);
   if (r != NULL)
   {
      r->setStatus(col, status);
   }
}

/**
 * Get status of given cell
 */
int Table::getStatus(int nRow, int nCol) const
{
   const TableRow *r = m_data->get(nRow);
   return (r != NULL) ? r->getStatus(nCol) : -1;
}

/**
 * Set object ID of given cell
 */
void Table::setCellObjectIdAt(int row, int col, UINT32 objectId)
{
   TableRow *r = m_data->get(row);
   if (r != NULL)
   {
      r->setCellObjectId(col, objectId);
   }
}

/**
 * Set base row for given row
 */
void Table::setBaseRowAt(int row, int baseRow)
{
   TableRow *r = m_data->get(row);
   if (r != NULL)
   {
      r->setBaseRow(baseRow);
   }
}

/**
 * Add all rows from another table.
 * Identical table format assumed.
 *
 * @param src source table
 */
void Table::addAll(Table *src)
{
   int numColumns = std::min(m_columns->size(), src->m_columns->size());
   for(int i = 0; i < src->m_data->size(); i++)
   {
      TableRow *dstRow = new TableRow(m_columns->size());
      TableRow *srcRow = src->m_data->get(i);
      for(int j = 0; j < numColumns; j++)
      {
         dstRow->set(j, srcRow->getValue(j), srcRow->getStatus(j), srcRow->getCellObjectId(j));
      }
      m_data->add(dstRow);
   }
}

/**
 * Copy one row from source table
 */
void Table::copyRow(Table *src, int row)
{
   TableRow *srcRow = src->m_data->get(row);
   if (srcRow == NULL)
      return;

   int numColumns = std::min(m_columns->size(), src->m_columns->size());
   TableRow *dstRow = new TableRow(m_columns->size());

   for(int j = 0; j < numColumns; j++)
   {
      dstRow->set(j, srcRow->getValue(j), srcRow->getStatus(j), srcRow->getCellObjectId(j));
   }

   m_data->add(dstRow);
}

/**
 * Build instance string
 */
void Table::buildInstanceString(int row, TCHAR *buffer, size_t bufLen)
{
   TableRow *r = m_data->get(row);
   if (r == NULL)
   {
      buffer[0] = 0;
      return;
   }

   String instance;
   bool first = true;
   for(int i = 0; i < m_columns->size(); i++)
   {
      if (m_columns->get(i)->isInstanceColumn())
      {
         if (!first)
            instance += _T("~~~");
         first = false;
         const TCHAR *value = r->getValue(i);
         if (value != NULL)
            instance += value;
      }
   }
   _tcslcpy(buffer, (const TCHAR *)instance, bufLen);
}

/**
 * Find row by instance value
 *
 * @return row number or -1 if no such row
 */
int Table::findRowByInstance(const TCHAR *instance)
{
   for(int i = 0; i < m_data->size(); i++)
   {
      TCHAR currInstance[256];
      buildInstanceString(i, currInstance, 256);
      if (!_tcscmp(instance, currInstance))
         return i;
   }
   return -1;
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
TableColumnDefinition::TableColumnDefinition(const TableColumnDefinition *src)
{
   m_name = _tcsdup(src->m_name);
   m_displayName = _tcsdup(src->m_displayName);
   m_dataType = src->m_dataType;
   m_instanceColumn = src->m_instanceColumn;
}

/**
 * Create table column definition from NXCP message
 */
TableColumnDefinition::TableColumnDefinition(const NXCPMessage *msg, UINT32 baseId)
{
   m_name = msg->getFieldAsString(baseId);
   if (m_name == NULL)
      m_name = _tcsdup(_T("(null)"));
   m_dataType = msg->getFieldAsUInt32(baseId + 1);
   m_displayName = msg->getFieldAsString(baseId + 2);
   if (m_displayName == NULL)
      m_displayName = _tcsdup(m_name);
   m_instanceColumn = msg->getFieldAsUInt16(baseId + 3) ? true : false;
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
void TableColumnDefinition::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, (UINT32)m_dataType);
   msg->setField(baseId + 2, m_displayName);
   msg->setField(baseId + 3, (WORD)(m_instanceColumn ? 1 : 0));
}
