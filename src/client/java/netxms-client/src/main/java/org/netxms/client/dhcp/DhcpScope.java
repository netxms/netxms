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

import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPMessage;

/**
 * DHCP scope (subnet or address pool) on DHCP server
 */
public class DhcpScope
{
   private String id;
   private InetAddressEx network;
   private String name;
   private String ranges;
   private boolean enabled;
   private long totalAddresses;
   private long usedAddresses;
   private long declinedAddresses;
   private long freeAddresses;
   private boolean declinedAddressesKnown;

   /**
    * Create scope object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public DhcpScope(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsString(baseId);
      network = msg.getFieldAsInetAddressEx(baseId + 1);
      name = msg.getFieldAsString(baseId + 2);
      ranges = msg.getFieldAsString(baseId + 3);
      enabled = msg.getFieldAsBoolean(baseId + 4);
      totalAddresses = msg.getFieldAsInt64(baseId + 5);
      usedAddresses = msg.getFieldAsInt64(baseId + 6);
      declinedAddresses = msg.getFieldAsInt64(baseId + 7);
      freeAddresses = msg.getFieldAsInt64(baseId + 8);
      declinedAddressesKnown = msg.getFieldAsBoolean(baseId + 9);
   }

   /**
    * Get scope ID (unique within DHCP server).
    *
    * @return scope ID
    */
   public String getId()
   {
      return id;
   }

   /**
    * Get scope network address.
    *
    * @return scope network address (may be invalid if network is not known)
    */
   public InetAddressEx getNetwork()
   {
      return network;
   }

   /**
    * Get scope name.
    *
    * @return scope name (empty string if not set)
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get address ranges in human readable form.
    *
    * @return address ranges
    */
   public String getRanges()
   {
      return ranges;
   }

   /**
    * Check if scope is enabled.
    *
    * @return true if scope is enabled
    */
   public boolean isEnabled()
   {
      return enabled;
   }

   /**
    * Get total number of addresses in scope.
    *
    * @return total number of addresses
    */
   public long getTotalAddresses()
   {
      return totalAddresses;
   }

   /**
    * Get number of addresses currently leased.
    *
    * @return number of used addresses
    */
   public long getUsedAddresses()
   {
      return usedAddresses;
   }

   /**
    * Get number of declined addresses. Valid only if {@link #isDeclinedAddressesKnown()} returns true.
    *
    * @return number of declined addresses
    */
   public long getDeclinedAddresses()
   {
      return declinedAddresses;
   }

   /**
    * Check if DHCP server reports number of declined addresses (some servers, like Microsoft DHCP Server or dnsmasq, do not).
    *
    * @return true if number of declined addresses is known
    */
   public boolean isDeclinedAddressesKnown()
   {
      return declinedAddressesKnown;
   }

   /**
    * Get number of free addresses.
    *
    * @return number of free addresses
    */
   public long getFreeAddresses()
   {
      return freeAddresses;
   }

   /**
    * Get scope utilization (used addresses as percentage of total).
    *
    * @return scope utilization in percents
    */
   public double getUtilization()
   {
      return (totalAddresses > 0) ? (double)usedAddresses * 100.0 / (double)totalAddresses : 0;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DhcpScope [id=" + id + ", network=" + network + ", name=" + name + ", ranges=" + ranges + ", enabled=" + enabled + ", totalAddresses=" + totalAddresses +
            ", usedAddresses=" + usedAddresses + ", declinedAddresses=" + declinedAddresses + ", freeAddresses=" + freeAddresses + "]";
   }
}
