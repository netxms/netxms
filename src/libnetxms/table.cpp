/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
TableRow::TableRow(int columnCount) : m_cells(columnCount, 8, Ownership::True)
{
   for(int i = 0; i < columnCount; i++)
      m_cells.add(new TableCell());
   m_objectId = 0;
   m_baseRow = -1;
}

/**
 * Table row copy constructor
 */
TableRow::TableRow(const TableRow& src) : m_cells(src.m_cells.size(), 8, Ownership::True)
{
   for(int i = 0; i < src.m_cells.size(); i++)
      m_cells.add(new TableCell(*src.m_cells.get(i)));
   m_objectId = src.m_objectId;
   m_baseRow = src.m_baseRow;
}

/**
 * Create empty table
 */
Table::Table() : m_data(32, 32, Ownership::True), m_columns(8, 8, Ownership::True)
{
   m_title = nullptr;
   m_source = DS_INTERNAL;
   m_extendedFormat = false;
}

/**
 * Create table from NXCP message
 */
Table::Table(const NXCPMessage& msg) : m_data(32, 32, Ownership::True), m_columns(8, 8, Ownership::True)
{
   createFromMessage(msg);
}

/**
 * Copy constructor
 */
Table::Table(const Table& src) : m_data(src.m_data.size(), 32, Ownership::True), m_columns(src.m_columns.size(), 8, Ownership::True)
{
   m_extendedFormat = src.m_extendedFormat;
   for(int i = 0; i < src.m_data.size(); i++)
      m_data.add(new TableRow(*src.m_data.get(i)));
   m_title = MemCopyString(src.m_title);
   m_source = src.m_source;
   for(int i = 0; i < src.m_columns.size(); i++)
      m_columns.add(new TableColumnDefinition(*src.m_columns.get(i)));
}

/**
 * Table destructor
 */
Table::~Table()
{
   MemFree(m_title);
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
   StringBuffer *buffer;
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
		   if (title != nullptr)
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
         wchar_t *displayName = (tmp != nullptr) ? WideStringFromUTF8String(tmp) : nullptr;
#else
         const char *name = CHECK_NULL_A(XMLGetAttr(attrs, "name"));
         const char *displayName = XMLGetAttr(attrs, "displayName");
#endif
         ps->table->addColumn(name, XMLGetAttrInt(attrs, "dataType", 0), displayName, XMLGetAttrBoolean(attrs, "isInstance", false));
		   ps->state = XML_STATE_COLUMN;
#ifdef UNICODE
         MemFree(name);
         MemFree(displayName);
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
      ps->buffer->appendUtf8String(s, len);
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
   state.buffer = new StringBuffer();

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
   char *compressedXml = nullptr;
   size_t compressedSize = 0;
   base64_decode_alloc(packedXml, strlen(packedXml), &compressedXml, &compressedSize);
   if (compressedXml == nullptr)
      return nullptr;

   size_t xmlSize = ntohl(*reinterpret_cast<uint32_t*>(compressedXml));
   char *xml = MemAllocStringA(xmlSize + 1);
   uLongf uncompSize = (uLongf)xmlSize;
   if (uncompress((BYTE *)xml, &uncompSize, (BYTE *)&compressedXml[4], (uLong)compressedSize - 4) != Z_OK)
   {
      MemFree(xml);
      MemFree(compressedXml);
      return nullptr;
   }
   xml[xmlSize] = 0;
   MemFree(compressedXml);

   Table *table = new Table();
   if (table->parseXML(xml))
   {
      MemFree(xml);
      return table;
   }
   MemFree(xml);
   delete table;
   return nullptr;
}

/**
 * Create JSON document from table
 */
json_t *Table::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "extendedFormat", json_boolean(m_extendedFormat));
   json_object_set_new(root, "source", json_integer(m_source));
   json_object_set_new(root, "title", json_string_t(m_title));

   json_t *columns = json_array();
   for(int i = 0; i < m_columns.size(); i++)
   {
      json_array_append_new(columns, m_columns.get(i)->toJson());
   }
   json_object_set_new(root, "columns", columns);

   json_t *data = json_array();
   for(int i = 0; i < m_data.size(); i++)
   {
      json_t *row = json_object();

      uint32_t objectId = m_data.get(i)->getObjectId();
      int baseRow = m_data.get(i)->getBaseRow();
      if (objectId != DEFAULT_OBJECT_ID)
      {
         json_object_set_new(row, "objectId", json_integer(objectId));
         if (baseRow != -1)
            json_object_set_new(row, "baseRow", json_integer(baseRow));
      }
      else if (baseRow != -1)
      {
         json_object_set_new(row, "baseRow", json_integer(baseRow));
      }

      json_t *values = json_array();
      for(int j = 0; j < m_columns.size(); j++)
      {
         json_t *cell = json_object();
         int status = m_data.get(i)->getStatus(j);
         if (status != DEFAULT_STATUS)
         {
            json_object_set_new(cell, "status", json_integer(status));
         }
         json_object_set_new(cell, "value", json_string_t(m_data.get(i)->getValue(j)));
         json_array_append_new(values, cell);
      }
      json_object_set_new(row, "values", values);

      json_array_append_new(data, row);
   }
   json_object_set_new(root, "data", data);

   return root;
}

/**
 * Create JSON document from table in Grafana format
 */
json_t *Table::toGrafanaJson() const
{
   json_t *data = json_array();
   for(int i = 0; i < m_data.size(); i++)
   {
      json_t *row = json_object();
      for(int j = 0; j < m_columns.size(); j++)
      {
         TableColumnDefinition *tableColumn = m_columns.get(j);
#ifdef UNICODE
         char *name = UTF8StringFromWideString(tableColumn->getDisplayName());
#else
         char *name = UTF8StringFromMBString(tableColumn->getDisplayName());
#endif
         if (tableColumn->getDataType() != DCI_DT_STRING)
         {
            String formattedValue = FormatDCIValue(tableColumn->getUnitName(), m_data.get(i)->getValue(j));
            json_object_set_new(row, name, json_string_t(formattedValue));
         }
         else
         {
            json_object_set_new(row, name, json_string_t(m_data.get(i)->getValue(j)));
         }
         MemFree(name);
      }
      json_array_append_new(data, row);
   }
   return data;
}

/**
 * Create XML document from table
 */
TCHAR *Table::toXML() const
{
   StringBuffer xml;
   xml.append(_T("<table extendedFormat=\""));
   xml.append(m_extendedFormat ? _T("true") : _T("false"));
   xml.append(_T("\" source=\""));
   xml.append(m_source);
   xml.append(_T("\" name=\""));
   xml.append(EscapeStringForXML2(m_title));
   xml.append(_T("\">\r\n<columns>\r\n"));
   for(int i = 0; i < m_columns.size(); i++)
   {
      TableColumnDefinition *column = m_columns.get(i);
      xml.append(_T("<column name=\""));
      xml.append(EscapeStringForXML2(column->getName()));
      xml.append(_T("\" displayName=\""));
      xml.append(EscapeStringForXML2(column->getDisplayName()));
      xml.append(_T("\" isInstance=\""));
      xml.append(column->isInstanceColumn() ? _T("true") : _T("false"));
      xml.append(_T("\" dataType=\""));
      xml.append(column->getDataType());
      xml.append(_T("\"/>\r\n"));
   }
   xml.append(_T("</columns>\r\n"));
   xml.append(_T("<data>\r\n"));
   for(int i = 0; i < m_data.size(); i++)
   {
      uint32_t objectId = m_data.get(i)->getObjectId();
      int baseRow = m_data.get(i)->getBaseRow();
      if (objectId != DEFAULT_OBJECT_ID)
      {
         xml.append(_T("<tr objectId=\""));
         xml.append(objectId);
         if (baseRow != -1)
         {
            xml.append(_T("\" baseRow=\""));
            xml.append(baseRow);
         }
         xml.append(_T("\">\r\n"));
      }
      else
      {
         if (baseRow != -1)
         {
            xml.append(_T("<tr baseRow=\""));
            xml.append(baseRow);
            xml.append(_T("\">\r\n"));
         }
         else
         {
            xml.append(_T("<tr>\r\n"));
         }
      }
      for(int j = 0; j < m_columns.size(); j++)
      {
         int status = m_data.get(i)->getStatus(j);
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
         xml.append(EscapeStringForXML2(m_data.get(i)->getValue(j)));
         xml.append(_T("</td>\r\n"));
      }
      xml.append(_T("</tr>\r\n"));
   }
   xml.append(_T("</data>\r\n"));
   xml.append(_T("</table>"));
   return MemCopyString(xml);
}

/**
 * Create packed XML document
 */
char *Table::toPackedXML() const
{
   TCHAR *xml = toXML();
   if (xml == nullptr)
      return nullptr;
   char *utf8xml = UTF8StringFromTString(xml);
   MemFree(xml);
   size_t len = strlen(utf8xml);
   uLongf buflen = compressBound((uLong)len);
   BYTE *buffer = MemAllocArrayNoInit<BYTE>(buflen + 4);
   if (compress(&buffer[4], &buflen, (BYTE *)utf8xml, (uLong)len) != Z_OK)
   {
      MemFree(utf8xml);
      MemFree(buffer);
      return nullptr;
   }
   MemFree(utf8xml);
   char *encodedBuffer = nullptr;
   *reinterpret_cast<uint32_t*>(buffer) = htonl(static_cast<uint32_t>(len));
   base64_encode_alloc(reinterpret_cast<char*>(buffer), buflen + 4, &encodedBuffer);
   MemFree(buffer);
   return encodedBuffer;
}

/**
 * Create table object from CSV file
 */
Table *Table::createFromCSV(const TCHAR *content, const TCHAR separator)
{
   if (content == nullptr)
   {
      return nullptr;
   }

   Table *table = new Table();

   TCHAR item[2048];
   bool success = true;
   int state, currIndex, pos, lineCount;
   const TCHAR *ptr2;
   StringList elements;
   for(ptr2 = content, currIndex = 0, state = 0, pos = 0, lineCount = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case _T('"'):
                  state = 1;     // String
                  break;
               case _T('\n'):
               case _T('\r'):
                  if (*(ptr2 + 1) == _T('\n'))
                  {
                     ptr2++;
                  }
                  item[pos] = 0;
                  if (lineCount == 0)
                  {
                     table->addColumn(item);
                  }
                  else
                  {
                     elements.add(item);
                     table->addRow();
                     for (int j = 0; j < elements.size(); j++)
                     {
                        table->set(j, elements.get(j));
                     }
                     elements.clear();
                  }
                  pos = 0;
                  currIndex = 0;
                  lineCount++;
                  break;
               case 0:
                  state = -1;       // Finish processing
                  break;
               default:
                  if (*ptr2 == separator)
                  {
                     item[pos] = 0;
                     if (lineCount == 0)
                     {
                        table->addColumn(item);
                     }
                     else
                     {

                        elements.add(item);
                     }
                     pos = 0;
                     currIndex++;
                  }
                  else if (pos < 2048 - 1)
                     item[pos++] = *ptr2;
            }
            break;
         case 1:  // String in ""
            switch(*ptr2)
            {
               case _T('"'):
                  if (*(ptr2 + 1) != _T('"'))
                  {
                     state = 0;     // Normal
                  }
                  else
                  {
                     ptr2++;
                     if (pos < 2048 - 1)
                        item[pos++] = *ptr2;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  success = false;  // Set error flag
                  break;
               default:
                  if (pos < 2048 - 1)
                     item[pos++] = *ptr2;
            }
            break;
      }
   }

   if (success)
   {
      if (elements.size() > 0)
      {
         table->addRow();
         for (int j = 0; j < elements.size(); j++)
         {
            table->set(j, elements.get(j));
         }
      }

      return table;
   }

   delete table;
   return NULL;
}

/**
 * Create table from NXCP message
 */
void Table::createFromMessage(const NXCPMessage& msg)
{
   int rows = msg.getFieldAsUInt32(VID_TABLE_NUM_ROWS);
   int columns = msg.getFieldAsUInt32(VID_TABLE_NUM_COLS);
   m_title = msg.getFieldAsString(VID_TABLE_TITLE);
   m_source = msg.getFieldAsInt16(VID_DCI_SOURCE_TYPE);
   m_extendedFormat = msg.getFieldAsBoolean(VID_TABLE_EXTENDED_FORMAT);

   uint32_t dwId = VID_TABLE_COLUMN_INFO_BASE;
   for(int i = 0; i < columns; i++, dwId += 10)
   {
      m_columns.add(new TableColumnDefinition(msg, dwId));
   }

   if (msg.isFieldExist(VID_INSTANCE_COLUMN))
   {
      TCHAR name[MAX_COLUMN_NAME];
      msg.getFieldAsString(VID_INSTANCE_COLUMN, name, MAX_COLUMN_NAME);
      for(int i = 0; i < m_columns.size(); i++)
      {
         if (!_tcsicmp(m_columns.get(i)->getName(), name))
         {
            m_columns.get(i)->setInstanceColumn(true);
            break;
         }
      }
   }

   dwId = VID_TABLE_DATA_BASE;
   for(int i = 0; i < rows; i++)
   {
      TableRow *row = new TableRow(columns);
      m_data.add(row);
      if (m_extendedFormat)
      {
         row->setObjectId(msg.getFieldAsUInt32(dwId++));
         if (msg.isFieldExist(dwId))
            row->setBaseRow(msg.getFieldAsInt32(dwId));
         dwId += 9;
      }
      for(int j = 0; j < columns; j++)
      {
         TCHAR *value = msg.getFieldAsString(dwId++);
         if (m_extendedFormat)
         {
            int status = msg.getFieldAsInt16(dwId++);
            UINT32 objectId = msg.getFieldAsUInt32(dwId++);
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
void Table::updateFromMessage(const NXCPMessage& msg)
{
   m_columns.clear();
   m_data.clear();
   MemFree(m_title);
	createFromMessage(msg);
}

/**
 * Fill NXCP message with table data
 */
int Table::fillMessage(NXCPMessage *msg, int offset, int rowLimit) const
{
	msg->setField(VID_TABLE_TITLE, CHECK_NULL_EX(m_title));
   msg->setField(VID_DCI_SOURCE_TYPE, static_cast<uint16_t>(m_source));
   msg->setField(VID_TABLE_EXTENDED_FORMAT, (UINT16)(m_extendedFormat ? 1 : 0));

	if (offset == 0)
	{
		msg->setField(VID_TABLE_NUM_ROWS, (UINT32)m_data.size());
		msg->setField(VID_TABLE_NUM_COLS, (UINT32)m_columns.size());

      uint32_t id = VID_TABLE_COLUMN_INFO_BASE;
      for(int i = 0; i < m_columns.size(); i++, id += 10)
         m_columns.get(i)->fillMessage(msg, id);
	}
	msg->setField(VID_TABLE_OFFSET, (UINT32)offset);

	int stopRow = (rowLimit == -1) ? m_data.size() : std::min(m_data.size(), offset + rowLimit);
   uint32_t id = VID_TABLE_DATA_BASE;
	for(int row = offset; row < stopRow; row++)
	{
      TableRow *r = m_data.get(row);
      if (m_extendedFormat)
      {
			msg->setField(id++, r->getObjectId());
         msg->setField(id++, r->getBaseRow());
         id += 8;
      }
		for(int col = 0; col < m_columns.size(); col++)
		{
			const TCHAR *tmp = r->getValue(col);
			msg->setField(id++, CHECK_NULL_EX(tmp));
         if (m_extendedFormat)
         {
            msg->setField(id++, (UINT16)r->getStatus(col));
            msg->setField(id++, r->getCellObjectId(col));
            id += 7;
         }
		}
	}
	msg->setField(VID_NUM_ROWS, (UINT32)(stopRow - offset));

	if (stopRow == m_data.size())
		msg->setEndOfSequence();
	return stopRow;
}

/**
 * Add new column
 */
int Table::addColumn(const TCHAR *name, int32_t dataType, const TCHAR *displayName, bool isInstance)
{
   m_columns.add(new TableColumnDefinition(name, displayName, dataType, isInstance));
   for(int i = 0; i < m_data.size(); i++)
      m_data.get(i)->addColumn();
	return m_columns.size() - 1;
}

/**
 * Add new column
 */
int Table::addColumn(const TableColumnDefinition& d)
{
   m_columns.add(new TableColumnDefinition(d));
   for(int i = 0; i < m_data.size(); i++)
      m_data.get(i)->addColumn();
   return m_columns.size() - 1;
}

/**
 * Add new column
 */
void Table::insertColumn(int index, const TCHAR *name, int32_t dataType, const TCHAR *displayName, bool isInstance)
{
   m_columns.insert(index, new TableColumnDefinition(name, displayName, dataType, isInstance));
   for(int i = 0; i < m_data.size(); i++)
      m_data.get(i)->insertColumn(index);
}

/**
 * Swap two columns
 */
void Table::swapColumns(int column1, int column2)
{
   m_columns.swap(column1, column2);
   for(int i = 0; i < m_data.size(); i++)
   {
      m_data.get(i)->swapColumns(column1, column2);
   }
}

/**
 * Get column index by name
 *
 * @param name column name
 * @return column index or -1 if there are no such column
 */
int Table::getColumnIndex(const TCHAR *name) const
{
   for(int i = 0; i < m_columns.size(); i++)
      if (!_tcsicmp(name, m_columns.get(i)->getName()))
         return i;
   return -1;
}

/**
 * Add new row
 */
int Table::addRow()
{
   return m_data.add(new TableRow(m_columns.size()));
}

/**
 * Insert new row
 */
int Table::insertRow(int insertBefore)
{
   if ((insertBefore < 0) || (insertBefore >= m_data.size()))
      return addRow();
   m_data.insert(insertBefore, new TableRow(m_columns.size()));
   return insertBefore;
}

/**
 * Delete row
 */
void Table::deleteRow(int row)
{
   m_data.remove(row);
}

/**
 * Delete column
 */
void Table::deleteColumn(int col)
{
   if ((col < 0) || (col >= m_columns.size()))
      return;

   m_columns.remove(col);
   for(int i = 0; i < m_data.size(); i++)
      m_data.get(i)->deleteColumn(col);
}

/**
 * Set data at position
 */
void Table::setAt(int nRow, int nCol, const TCHAR *value)
{
   TableRow *r = m_data.get(nRow);
   if (r != NULL)
   {
      r->setValue(nCol, value);
   }
}

/**
 * Set pre-allocated data at position
 */
void Table::setPreallocatedAt(int nRow, int nCol, TCHAR *value)
{
   TableRow *r = m_data.get(nRow);
   if (r != NULL)
   {
      r->setPreallocatedValue(nCol, value);
   }
   else
   {
      MemFree(value);
   }
}

/**
 * Set integer data at position
 */
void Table::setAt(int nRow, int nCol, int32_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%d"), value);
   setAt(nRow, nCol, buffer);
}

/**
 * Set unsigned integer data at position
 */
void Table::setAt(int nRow, int nCol, uint32_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%u"), value);
   setAt(nRow, nCol, buffer);
}

/**
 * Set 64 bit integer data at position
 */
void Table::setAt(int nRow, int nCol, int64_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, INT64_FMT, value);
   setAt(nRow, nCol, buffer);
}

/**
 * Set unsigned 64 bit integer data at position
 */
void Table::setAt(int nRow, int nCol, uint64_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, UINT64_FMT, value);
   setAt(nRow, nCol, buffer);
}

/**
 * Set floating point data at position
 */
void Table::setAt(int nRow, int nCol, double value, int digits)
{
   TCHAR buffer[32];
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(buffer, 32, _TRUNCATE, _T("%1.*f"), digits, value);
#else
   _sntprintf(buffer, 32, _T("%1.*f"), digits, value);
#endif
   setAt(nRow, nCol, buffer);
}

/**
 * Get value of given cell as string
 */
const TCHAR *Table::getAsString(int nRow, int nCol, const TCHAR *defaultValue) const
{
   const TableRow *r = m_data.get(nRow);
   if (r == nullptr)
      return defaultValue;
   const TCHAR *v = r->getValue(nCol);
   return (v != nullptr) ? v : defaultValue;
}

/**
 * Get value of given cell as 32 bit integer
 */
int32_t Table::getAsInt(int nRow, int nCol) const
{
   const TCHAR *v = getAsString(nRow, nCol);
   return v != nullptr ? _tcstol(v, nullptr, 0) : 0;
}

/**
 * Get value of given cell as 32 bit unsigned integer
 */
uint32_t Table::getAsUInt(int nRow, int nCol) const
{
   const TCHAR *v = getAsString(nRow, nCol);
   return v != nullptr ? _tcstoul(v, nullptr, 0) : 0;
}

/**
 * Get value of given cell as 64 bit integer
 */
int64_t Table::getAsInt64(int nRow, int nCol) const
{
   const TCHAR *v = getAsString(nRow, nCol);
   return v != nullptr ? _tcstoll(v, nullptr, 0) : 0;
}

/**
 * Get value of given cell as 64 bit unsigned integer
 */
uint64_t Table::getAsUInt64(int nRow, int nCol) const
{
   const TCHAR *v = getAsString(nRow, nCol);
   return v != nullptr ? _tcstoull(v, nullptr, 0) : 0;
}

/**
 * Get value of given cell as floating point number
 */
double Table::getAsDouble(int nRow, int nCol) const
{
   const TCHAR *v = getAsString(nRow, nCol);
   return v != nullptr ? _tcstod(v, nullptr) : 0;
}

/**
 * Set status of given cell
 */
void Table::setStatusAt(int row, int col, int status)
{
   TableRow *r = m_data.get(row);
   if (r != nullptr)
   {
      r->setStatus(col, status);
   }
}

/**
 * Get status of given cell
 */
int Table::getStatus(int nRow, int nCol) const
{
   const TableRow *r = m_data.get(nRow);
   return (r != nullptr) ? r->getStatus(nCol) : -1;
}

/**
 * Set object ID of given cell
 */
void Table::setCellObjectIdAt(int row, int col, uint32_t objectId)
{
   TableRow *r = m_data.get(row);
   if (r != nullptr)
   {
      r->setCellObjectId(col, objectId);
   }
}

/**
 * Set base row for given row
 */
void Table::setBaseRowAt(int row, int baseRow)
{
   TableRow *r = m_data.get(row);
   if (r != nullptr)
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
void Table::addAll(const Table *src)
{
   int numColumns = std::min(m_columns.size(), src->m_columns.size());
   for(int i = 0; i < src->m_data.size(); i++)
   {
      TableRow *dstRow = new TableRow(m_columns.size());
      TableRow *srcRow = src->m_data.get(i);
      for(int j = 0; j < numColumns; j++)
      {
         dstRow->set(j, srcRow->getValue(j), srcRow->getStatus(j), srcRow->getCellObjectId(j));
      }
      m_data.add(dstRow);
   }
}

/**
 * Copy one row from source table
 */
int Table::copyRow(const Table *src, int row)
{
   TableRow *srcRow = src->m_data.get(row);
   if (srcRow == NULL)
      return -1;

   int numColumns = std::min(m_columns.size(), src->m_columns.size());
   TableRow *dstRow = new TableRow(m_columns.size());

   for(int j = 0; j < numColumns; j++)
   {
      dstRow->set(j, srcRow->getValue(j), srcRow->getStatus(j), srcRow->getCellObjectId(j));
   }

   return m_data.add(dstRow);
}

/**
 * Merge given table - add missing columns as necessary and place data into columns by column name
 */
void Table::merge(const Table *src)
{
   // Create column index translation and add missing columns
   int numSrcColumns = src->m_columns.size();
   int *tran = static_cast<int*>(MemAllocLocal(numSrcColumns * sizeof(int)));
   for(int i = 0; i < numSrcColumns; i++)
   {
      TableColumnDefinition *sc = src->m_columns.get(i);
      int idx = getColumnIndex(sc->getName());
      if (idx == -1)
      {
         idx = addColumn(*sc);
      }
      tran[i] = idx;
   }

   for(int r = 0; r < src->m_data.size(); r++)
   {
      TableRow *dstRow = new TableRow(m_columns.size());
      TableRow *srcRow = src->m_data.get(r);
      for(int c = 0; c < numSrcColumns; c++)
      {
         dstRow->set(tran[c], srcRow->getValue(c), srcRow->getStatus(c), srcRow->getCellObjectId(c));
      }
      m_data.add(dstRow);
   }

   MemFreeLocal(tran);
}

/**
 * Merge single row from given table - add missing columns as necessary and place data into columns by column name
 */
int Table::mergeRow(const Table *src, int row, int insertBefore)
{
   TableRow *srcRow = src->m_data.get(row);
   if (srcRow == nullptr)
      return -1;

   // Create column index translation and add missing columns
   int numSrcColumns = src->m_columns.size();
   int *tran = static_cast<int*>(MemAllocLocal(numSrcColumns * sizeof(int)));
   for(int i = 0; i < numSrcColumns; i++)
   {
      TableColumnDefinition *sc = src->m_columns.get(i);
      int idx = getColumnIndex(sc->getName());
      if (idx == -1)
      {
         idx = addColumn(*sc);
      }
      tran[i] = idx;
   }

   TableRow *dstRow = new TableRow(m_columns.size());
   for(int c = 0; c < numSrcColumns; c++)
   {
      dstRow->set(tran[c], srcRow->getValue(c), srcRow->getStatus(c), srcRow->getCellObjectId(c));
   }

   MemFreeLocal(tran);

   if ((insertBefore >= 0) && (insertBefore < m_data.size()))
   {
      m_data.insert(insertBefore, dstRow);
      return insertBefore;
   }

   return m_data.add(dstRow);
}

/**
 * Build instance string
 */
void Table::buildInstanceString(int row, TCHAR *buffer, size_t bufLen)
{
   TableRow *r = m_data.get(row);
   if (r == nullptr)
   {
      buffer[0] = 0;
      return;
   }

   StringBuffer instance;
   bool first = true;
   for(int i = 0; i < m_columns.size(); i++)
   {
      if (m_columns.get(i)->isInstanceColumn())
      {
         if (!first)
            instance += _T("~~~");
         first = false;
         const TCHAR *value = r->getValue(i);
         if (value != nullptr)
            instance += value;
      }
   }
   if (instance.isEmpty())
   {
      instance.append(_T("#"));
      instance.append(row);
   }
   _tcslcpy(buffer, instance.cstr(), bufLen);
}

/**
 * Find row by instance value
 *
 * @return row number or -1 if no such row
 */
int Table::findRowByInstance(const TCHAR *instance)
{
   for(int i = 0; i < m_data.size(); i++)
   {
      TCHAR currInstance[1024];
      buildInstanceString(i, currInstance, 1024);
      if (!_tcscmp(instance, currInstance))
         return i;
   }
   return -1;
}

/**
 * Find row using given comparator and key
 */
int Table::findRow(void *key, bool (*comparator)(const TableRow *, void *))
{
   for(int i = 0; i < m_data.size(); i++)
   {
      if (comparator(m_data.get(i), key))
         return i;
   }
   return -1;
}

/**
 * Display table on terminal
 */
void Table::writeToTerminal() const
{
   // calculate column widths and print headers
   Buffer<int, 128> widths(m_columns.size());
   WriteToTerminal(_T("\x1b[1m|"));
   for(int c = 0; c < m_columns.size(); c++)
   {
      widths[c] = static_cast<int>(_tcslen(m_columns.get(c)->getName()));
      for(int i = 0; i < m_data.size(); i++)
      {
         int len = static_cast<int>(_tcslen(getAsString(i, c, _T(""))));
         if (len > widths[c])
            widths[c] = len;
      }
      WriteToTerminalEx(_T(" %*s |"), -widths[c], m_columns.get(c)->getName());
   }

   WriteToTerminal(_T("\n"));
   for(int i = 0; i < m_data.size(); i++)
   {
      WriteToTerminal(_T("\x1b[1m|\x1b[0m"));
      for(int j = 0; j < m_columns.size(); j++)
      {
         if (m_columns.get(j)->isInstanceColumn())
            WriteToTerminalEx(_T(" \x1b[32;1m%*s\x1b[0m \x1b[1m|\x1b[0m"), -widths[j], getAsString(i, j, _T("")));
         else
            WriteToTerminalEx(_T(" %*s \x1b[1m|\x1b[0m"), -widths[j], getAsString(i, j, _T("")));
      }
      WriteToTerminal(_T("\n"));
   }
}

/**
 * Dump table using given output stream
 */
void Table::dump(FILE *out, bool withHeader, TCHAR delimiter) const
{
   if (m_columns.isEmpty())
      return;

   if (withHeader)
   {
      _fputts(getColumnName(0), out);
      for(int c = 1; c < m_columns.size(); c++)
      {
         _fputtc(delimiter, out);
         _fputts(getColumnName(c), out);
      }
      _fputtc(_T('\n'), out);
   }

   for(int i = 0; i < m_data.size(); i++)
   {
      _fputts(getAsString(i, 0, _T("")), out);
      for(int j = 1; j < m_columns.size(); j++)
      {
         _fputtc(delimiter, out);
         _fputts(getAsString(i, j, _T("")), out);
      }
      _fputtc(_T('\n'), out);
   }
}

/**
 * Dump table to log
 */
void Table::dump(const TCHAR *tag, int level, const TCHAR *prefix, bool withHeader, TCHAR delimiter) const
{
   if (m_columns.isEmpty())
      return;

   StringBuffer sb;
   if (withHeader)
   {
      sb.append(getColumnName(0));
      for (int c = 1; c < m_columns.size(); c++)
      {
         sb.append(delimiter);
         sb.append(getColumnName(c));
      }
      nxlog_debug_tag(tag, level, _T("%s%s"), prefix, sb.cstr());
   }

   for (int i = 0; i < m_data.size(); i++)
   {
      sb.clear();
      sb.append(getAsString(i, 0, _T("")));
      for (int j = 1; j < m_columns.size(); j++)
      {
         sb.append(delimiter);
         sb.append(getAsString(i, j, _T("")));
      }
      nxlog_debug_tag(tag, level, _T("%s%s"), prefix, sb.cstr());
   }
}

/**
 * Create new table column definition
 */
TableColumnDefinition::TableColumnDefinition(const TCHAR *name, const TCHAR *displayName, INT32 dataType, bool isInstance)
{
   _tcslcpy(m_name, CHECK_NULL(name), MAX_COLUMN_NAME);
   _tcslcpy(m_displayName, (displayName != nullptr) ? displayName : m_name, MAX_DB_STRING);
   m_dataType = dataType;
   m_instanceColumn = isInstance;
   m_unitName[0] = 0;
   m_multiplier = 0;
   m_useMultiplier = 0;
}

/**
 * Create table column definition from NXCP message
 */
TableColumnDefinition::TableColumnDefinition(const NXCPMessage& msg, uint32_t baseId)
{
   msg.getFieldAsString(baseId, m_name, MAX_COLUMN_NAME);
   m_dataType = msg.getFieldAsInt32(baseId + 1);
   msg.getFieldAsString(baseId + 2, m_displayName, MAX_DB_STRING);
   if (m_displayName[0] == 0)
      _tcscpy(m_displayName, m_name);
   m_instanceColumn = msg.getFieldAsBoolean(baseId + 3);
   m_unitName[0] = 0;
   m_multiplier = 0;
   m_useMultiplier = 0;
}

/**
 * Fill message with table column definition data
 */
void TableColumnDefinition::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, m_dataType);
   msg->setField(baseId + 2, m_displayName);
   msg->setField(baseId + 3, m_instanceColumn);
   msg->setField(baseId + 4, m_unitName);
   msg->setField(baseId + 5, m_multiplier);
   msg->setField(baseId + 6, m_useMultiplier);
}

/**
 * Create JSON document
 */
json_t *TableColumnDefinition::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "name", json_string_t(m_name));
   json_object_set_new(json, "dataType", json_integer(m_dataType));
   json_object_set_new(json, "displayName", json_string_t(m_displayName));
   json_object_set_new(json, "instanceColumn", json_boolean(m_instanceColumn));
   json_object_set_new(json, "unitName", json_string_t(m_unitName));
   json_object_set_new(json, "multiplier", json_integer(m_multiplier));
   json_object_set_new(json, "useMultiplier", json_integer(m_useMultiplier));
   return json;
}

/**
 * Set display name for column
 */
void TableColumnDefinition::setDisplayName(const TCHAR *name)
{
   _tcslcpy(m_displayName, CHECK_NULL_EX(name), MAX_DB_STRING);
}

/**
 * Set display name for column
 */
void TableColumnDefinition::setUnitName(const TCHAR *name)
{
   _tcslcpy(m_unitName, CHECK_NULL_EX(name), 63);
}
