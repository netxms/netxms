/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
InetAddressListElement::InetAddressListElement(NXCPMessage *msg, UINT32 baseId)
{
   m_type = msg->getFieldAsInt16(baseId);
   m_baseAddress = msg->getFieldAsInetAddress(baseId + 1);
   if (m_type == InetAddressListElement_SUBNET)
   {
      m_baseAddress.setMaskBits(msg->getFieldAsInt16(baseId + 2));
   }
   else
   {
      m_endAddress = msg->getFieldAsInetAddress(baseId + 2);
   }
}

/**
 * Create new "range" type address list element
 */
InetAddressListElement::InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr)
{
   m_type = InetAddressListElement_RANGE;
   m_baseAddress = baseAddr;
   m_endAddress = endAddr;
}

/**
 * Create new "subnet" type address list element
 */
InetAddressListElement::InetAddressListElement(const InetAddress& baseAddr, int maskBits)
{
   m_type = InetAddressListElement_SUBNET;
   m_baseAddress = baseAddr;
   m_baseAddress.setMaskBits(maskBits);
}

/**
 * Create address list element from DB record
 * Expected field order: addr_type,addr1,addr2
 */
InetAddressListElement::InetAddressListElement(DB_RESULT hResult, int row)
{
   m_type = DBGetFieldLong(hResult, row, 0);
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
}

/**
 * Fill NXCP message
 */
void InetAddressListElement::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
   msg->setField(baseId, (INT16)m_type);
   msg->setField(baseId + 1, m_baseAddress);
   if (m_type == InetAddressListElement_SUBNET)
      msg->setField(baseId + 2, (INT16)m_baseAddress.getMaskBits());
   else
      msg->setField(baseId + 2, m_endAddress);
}

/**
 * Check if element contains given address
 */
bool InetAddressListElement::contains(const InetAddress& addr) const
{
   if (m_type == InetAddressListElement_SUBNET)
      return m_baseAddress.contain(addr);
   if ((m_baseAddress.getFamily() == addr.getFamily()) && (m_endAddress.getFamily() == addr.getFamily()))
   {
      InetAddress a = addr;
      a.setMaskBits(m_baseAddress.getMaskBits());   // compare only address parts
      return (m_baseAddress.compareTo(a) <= 0) && (m_endAddress.compareTo(a) >= 0);
   }
   return false;
}

/**
 * Convert to text representation
 */
String InetAddressListElement::toString() const
{
   String s = m_baseAddress.toString();
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
   return s;
}

/**
 * Update address list from NXCP message
 */
bool UpdateAddressListFromMessage(NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   int listType = msg->getFieldAsInt32(VID_ADDR_LIST_TYPE);
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM address_lists WHERE list_type=%d"), listType);

   DBBegin(hdb);
   bool success = DBQuery(hdb, query);
   if (success)
   {
      int count = msg->getFieldAsInt32(VID_NUM_RECORDS);
      UINT32 fieldId = VID_ADDR_LIST_BASE;
      for(int i = 0; (i < count) && success; i++, fieldId += 10)
      {
         InetAddressListElement e = InetAddressListElement(msg, fieldId);
         if (e.getType() == InetAddressListElement_SUBNET)
         {
            TCHAR baseAddr[64];
            _sntprintf(query, 256, _T("INSERT INTO address_lists (list_type,addr_type,addr1,addr2,community_id) VALUES (%d,%d,'%s','%d',0)"),
                       listType, e.getType(), e.getBaseAddress().toString(baseAddr), e.getBaseAddress().getMaskBits());
         }
         else
         {
            TCHAR baseAddr[64], endAddr[64];
            _sntprintf(query, 256, _T("INSERT INTO address_lists (list_type,addr_type,addr1,addr2,community_id) VALUES (%d,%d,'%s','%s',0)"),
                       listType, e.getType(), e.getBaseAddress().toString(baseAddr), e.getEndAddress().toString(endAddr));
         }
         success = DBQuery(hdb, query);
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
