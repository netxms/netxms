/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.dhcp;

import java.util.Date;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DhcpLeaseState;
import org.netxms.client.constants.DhcpLeaseType;

/**
 * DHCP lease (or reservation) on DHCP server
 */
public class DhcpLease
{
   private InetAddressEx ipAddress;
   private String scopeId;
   private MacAddress macAddress;
   private String clientId;
   private String hostname;
   private DhcpLeaseType type;
   private DhcpLeaseState state;
   private Date lastTransaction;
   private Date expiration;

   /**
    * Create lease object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public DhcpLease(NXCPMessage msg, long baseId)
   {
      ipAddress = msg.getFieldAsInetAddressEx(baseId);
      scopeId = msg.getFieldAsString(baseId + 1);
      macAddress = msg.getFieldAsMacAddress(baseId + 2);
      clientId = msg.getFieldAsString(baseId + 3);
      hostname = msg.getFieldAsString(baseId + 4);
      type = DhcpLeaseType.getByValue(msg.getFieldAsInt32(baseId + 5));
      state = DhcpLeaseState.getByValue(msg.getFieldAsInt32(baseId + 6));
      lastTransaction = getOptionalDate(msg, baseId + 7);
      expiration = getOptionalDate(msg, baseId + 8);
   }

   /**
    * Get timestamp field where 0 means "not set".
    */
   private static Date getOptionalDate(NXCPMessage msg, long fieldId)
   {
      long t = msg.getFieldAsInt64(fieldId);
      return (t != 0) ? new Date(t * 1000) : null;
   }

   /**
    * Get leased IP address. For IPv6 prefix delegation mask is set to delegated prefix length.
    *
    * @return leased IP address
    */
   public InetAddressEx getIpAddress()
   {
      return ipAddress;
   }

   /**
    * Check if this lease is IPv6 prefix delegation.
    *
    * @return true if this lease is IPv6 prefix delegation
    */
   public boolean isPrefixDelegation()
   {
      return (ipAddress != null) && (ipAddress.getAddress() != null) && (ipAddress.getAddress().getAddress().length == 16) && (ipAddress.getMask() < 128);
   }

   /**
    * Get ID of scope this lease belongs to.
    *
    * @return scope ID (empty string if unknown)
    */
   public String getScopeId()
   {
      return scopeId;
   }

   /**
    * Get client MAC address.
    *
    * @return client MAC address (may be null or empty)
    */
   public MacAddress getMacAddress()
   {
      return macAddress;
   }

   /**
    * Get DHCPv4 client identifier or DHCPv6 DUID.
    *
    * @return client identifier
    */
   public String getClientId()
   {
      return clientId;
   }

   /**
    * Get client host name.
    *
    * @return client host name
    */
   public String getHostname()
   {
      return hostname;
   }

   /**
    * Get lease type.
    *
    * @return lease type
    */
   public DhcpLeaseType getType()
   {
      return type;
   }

   /**
    * Get lease state.
    *
    * @return lease state
    */
   public DhcpLeaseState getState()
   {
      return state;
   }

   /**
    * Get time of last transaction with client.
    *
    * @return time of last transaction or null if unknown
    */
   public Date getLastTransaction()
   {
      return lastTransaction;
   }

   /**
    * Get lease expiration time.
    *
    * @return lease expiration time or null if lease does not expire or expiration time is unknown
    */
   public Date getExpiration()
   {
      return expiration;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DhcpLease [ipAddress=" + ipAddress + ", scopeId=" + scopeId + ", macAddress=" + macAddress + ", clientId=" + clientId + ", hostname=" + hostname + ", type=" + type +
            ", state=" + state + ", lastTransaction=" + lastTransaction + ", expiration=" + expiration + "]";
   }
}
