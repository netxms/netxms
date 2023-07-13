/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.base;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;

/**
 * Inet address wrapper
 */
public class InetAddressEx
{
   protected InetAddress address;
   protected int mask;
   
   /**
    * Create from InetAddress object with default network mask (/32 for IPv4 and /128 for IPv6).
    *
    * @param address IP address
    */
   public InetAddressEx(InetAddress address)
   {
      this.address = address;
      this.mask = (address instanceof Inet4Address) ? 32 : 128;
   }

   /**
    * Create from InetAddress with given network mask.
    *
    * @param address IP address
    * @param mask network mask bits
    */
   public InetAddressEx(InetAddress address, int mask)
   {
      this.address = address;
      this.mask = mask;
   }

   /**
    * Create from InetAddress with given network mask.
    *
    * @param address IP address
    * @param mask network mask
    */
   public InetAddressEx(InetAddress address, InetAddress mask)
   {
      this.address = address;
      this.mask = bitsInMask(mask);
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public InetAddressEx(InetAddressEx src)
   {
      this.address = src.address;
      this.mask = src.mask;
   }
   
   /**
    * Create AF_UNSPEC address
    */
   public InetAddressEx()
   {
      address = null;
      mask = 0;
   }
   
   /**
    * Check if address is a valid IPv4/IPv6 address
    * 
    * @return true if address is a valid IPv4/IPv6 address
    */
   public boolean isValidAddress()
   {
      return address != null;
   }
   
   /**
    * Check if address is a valid unicast address
    * 
    * @return true if address is a valid unicast address
    */
   public boolean isValidUnicastAddress()
   {
      return (address != null) && 
            !address.isAnyLocalAddress() && 
            !address.isLinkLocalAddress() && 
            !address.isLoopbackAddress() && 
            !address.isMulticastAddress();
   }
   
   /**
    * Get host address as text (without mask length)
    * 
    * @return host address as text (without mask length)
    */
   public String getHostAddress()
   {
      return (address != null) ? address.getHostAddress().replaceFirst("(^|:)(0+(:|$)){2,8}", "::") : "UNSPEC";
   }

   /**
    * Get IP address object
    * 
    * @return IP address
    */
   public InetAddress getAddress()
   {
      return address;
   }

   /**
    * Get network mask (bit length)
    * 
    * @return network mask bit length
    */
   public int getMask()
   {
      return mask;
   }
   
   /**
    * Set IP address
    * 
    * @param address IP address to set
    */
   public void setAddress(InetAddress address)
   {
      this.address = address;
   }

   /**
    * Set mask length
    * 
    * @param mask new mask length
    */
   public void setMask(int mask)
   {
      this.mask = mask;
   }

   /**
    * Get address bytes
    * 
    * @return address bytes
    */
   public byte[] getAddressBytes()
   {
      return (address != null) ? address.getAddress() : new byte[0];
   }
   
   /**
    * Get number of bits in host part of the address (address length - mask length).
    * 
    * @return number of bits in host part of the address
    */
   public int getHostBits()
   {
      if (address instanceof Inet4Address)
         return 32 - mask;
      if (address instanceof Inet6Address)
         return 128 - mask;
      return 0;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return (address != null) ? address.getHostAddress().replaceFirst("(^|:)(0+(:|$)){2,8}", "::") + "/" + mask : "UNSPEC";
   }
   
   /**
    * Calculate number of bits in network mask
    * 
    * @param mask network mask as IP address
    * @return network mask length (in bits)
    */
   public static int bitsInMask(InetAddress mask)
   {
      int bits = 0;
      byte[] bytes = mask.getAddress();
      for(byte b : bytes)
      {
         if (b == 0xFF)
         {
            bits += 8;
            continue;
         }
         for(int m = 0x80; m != 0; m = m >> 1)
         {
            if ((b & m) != m)
               break;
            bits++;
         }
      }
      return bits;
   }

   /**
    * Convert bit count into mask
    * 
    * @return network mask as IP address
    */
   public InetAddress maskFromBits()
   {
      if (address == null)
         return null;

      try
      {
         byte[] bytes = new byte[(address instanceof Inet4Address) ? 4 : 16];
         int bits = mask;
         int i = 0;
         while(bits > 8)
         {
            bytes[i++] = (byte)0xFF;
            bits -= 8;
         }
         byte b = (byte)0x80;
         while(bits-- > 0)
         {
            bytes[i] |= b;
            b = (byte)(b >> 1);
         }
         return InetAddress.getByAddress(bytes);
      }
      catch(Exception e)
      {
         return null;
      }
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + ((address == null) ? 0 : address.hashCode());
      result = prime * result + mask;
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      InetAddressEx other = (InetAddressEx)obj;
      if (address == null)
      {
         if (other.address != null)
            return false;
      }
      else if (!address.equals(other.address))
         return false;
      if (mask != other.mask)
         return false;
      return true;
   }
}
