/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: addrlist.cpp
**
**/

#include "nxcore.h"

/**
 * Create address list element from NXCP message
 */
InetAddressListElement::InetAddressListElement(const NXCPMessage& msg, uint32_t baseId)
{
   m_type = (AddressListElementType)msg.getFieldAsInt16(baseId);
   m_baseAddress = msg.getFieldAsInetAddress(baseId + 1);
   if (m_type == InetAddressListElement_SUBNET)
   {
      m_baseAddress.setMaskBits(msg.getFieldAsInt16(baseId + 2));
   }
   else
   {
      m_endAddress = msg.getFieldAsInetAddress(baseId + 2);
   }
   m_zoneUIN = msg.getFieldAsInt32(baseId + 3);
   m_proxyId = msg.getFieldAsInt32(baseId + 4);
   msg.getFieldAsString(baseId + 5, m_comments, MAX_ADDRESS_LIST_COMMENTS_LEN);
}

/**
 * Create new "range" type address list element
 */
InetAddressListElement::InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr)
{
   m_type = InetAddressListElement_RANGE;
   m_baseAddress = baseAddr;
   m_endAddress = endAddr;
   m_zoneUIN = 0;
   m_proxyId = 0;
   m_comments[0] = 0;
}

/**
 * Create new "subnet" type address list element
 */
InetAddressListElement::InetAddressListElement(const InetAddress& baseAddr, int maskBits)
{
   m_type = InetAddressListElement_SUBNET;
   m_baseAddress = baseAddr;
   m_baseAddress.setMaskBits(maskBits);
   m_zoneUIN = 0;
   m_proxyId = 0;
   m_comments[0] = 0;
}

/**
 * Create new "range" type address list element with zone and proxy id
 */
InetAddressListElement::InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr, int32_t zoneUIN, uint32_t proxyId)
{
   m_type = InetAddressListElement_RANGE;
   m_baseAddress = baseAddr;
   m_endAddress = endAddr;
   m_zoneUIN = zoneUIN;
   m_proxyId = proxyId;
   m_comments[0] = 0;
}

/**
 * Create address list element from DB record
 * Expected field order: addr_type,addr1,addr2,zone_uin,proxy_id,comment
 */
InetAddressListElement::InetAddressListElement(DB_RESULT hResult, int row)
{
   m_type = (AddressListElementType)DBGetFieldLong(hResult, row, 0);
   m_baseAddress = DBGetFieldInetAddr(hResult, row, 1);
   if (m_type == InetAddressListElement_SUBNET)
   {
      // mask field can be represented as bit count or as IPv4 mask (like 255.255.255.0)
      TCHAR mask[64];
      DBGetField(hResult, row, 2, mask, 64);

      TCHAR *eptr;
      int bits = _tcstol(mask, &eptr, 10);
      if ((bits != 0) && (*eptr == 0))
      {
         m_baseAddress.setMaskBits(bits);
      }
      else if (m_baseAddress.getFamily() == AF_INET)
      {
         InetAddress a = InetAddress::parse(mask);
         if (a.getFamily() == AF_INET)
         {
            m_baseAddress.setMaskBits(BitsInMask(a.getAddressV4()));
         }
      }
   }
   else
   {
      m_endAddress = DBGetFieldInetAddr(hResult, row, 2);
   }
   m_zoneUIN = DBGetFieldLong(hResult, row, 3);
   m_proxyId = DBGetFieldLong(hResult, row, 4);
   DBGetField(hResult, row, 5, m_comments, MAX_ADDRESS_LIST_COMMENTS_LEN);
}

/**
 * Fill NXCP message
 */
void InetAddressListElement::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, static_cast<int16_t>(m_type));
   msg->setField(baseId + 1, m_baseAddress);
   if (m_type == InetAddressListElement_SUBNET)
      msg->setField(baseId + 2, static_cast<int16_t>(m_baseAddress.getMaskBits()));
   else
      msg->setField(baseId + 2, m_endAddress);
   msg->setField(baseId + 3, m_zoneUIN);
   msg->setField(baseId + 4, m_proxyId);
   msg->setField(baseId + 5, m_comments);
}

/**
 * Check if element contains given address
 */
bool InetAddressListElement::contains(const InetAddress& addr) const
{
   return (m_type == InetAddressListElement_SUBNET) ? m_baseAddress.contains(addr) : addr.inRange(m_baseAddress, m_endAddress);
}

/**
 * Convert to text representation
 */
String InetAddressListElement::toString() const
{
   StringBuffer s = m_baseAddress.toString();
   if (m_type == InetAddressListElement_SUBNET)
   {
      s.append(_T('/'));
      s.append(m_baseAddress.getMaskBits());
   }
   else
   {
      s.append(_T('-'));
      s.append(m_endAddress.toString());
   }
   if (m_zoneUIN != 0)
   {
      s.append(_T('@'));
      s.append(m_zoneUIN);
   }
   return s;
}

/**
 * Update address list from NXCP message
 */
bool UpdateAddressListFromMessage(const NXCPMessage& msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   uint32_t listType = msg.getFieldAsUInt32(VID_ADDR_LIST_TYPE);

   DBBegin(hdb);

   bool success = ExecuteQueryOnObject(hdb, listType, _T("DELETE FROM address_lists WHERE list_type=?"));
   if (success)
   {
      int count = msg.getFieldAsInt32(VID_NUM_RECORDS);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO address_lists (list_type,addr_type,addr1,addr2,community_id,zone_uin,proxy_id,comments) VALUES (?,?,?,?,?,?,?,?)"), count > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, listType);

            uint32_t fieldId = VID_ADDR_LIST_BASE;
            for(int i = 0; (i < count) && success; i++, fieldId += 10)
            {
               InetAddressListElement e = InetAddressListElement(msg, fieldId);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e.getType());
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, e.getBaseAddress());
               if (e.getType() == InetAddressListElement_SUBNET)
                  DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, e.getBaseAddress().getMaskBits());
               else
                  DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, e.getEndAddress());
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)0);
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, e.getZoneUIN());
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, e.getProxyId());
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, e.getComments(), DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }

            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         DBCommit(hdb);
      }
      else
      {
         DBRollback(hdb);
      }
   }
   else
   {
      DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Load server address list of given type
 */
ObjectArray<InetAddressListElement> *LoadServerAddressList(int listType)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[256];
   _sntprintf(query, 256, _T("SELECT addr_type,addr1,addr2,zone_uin,proxy_id,comments FROM address_lists WHERE list_type=%d"), listType);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return nullptr;
   }

   int count = DBGetNumRows(hResult);
   ObjectArray<InetAddressListElement> *list = new ObjectArray<InetAddressListElement>(count, 16, Ownership::True);

   for(int i = 0; i < count; i++)
   {
      list->add(new InetAddressListElement(hResult, i));
   }

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
   return list;
}
