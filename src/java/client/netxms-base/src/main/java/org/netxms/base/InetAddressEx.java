/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.net.InetAddress;

/**
 * Inet address wrapper
 */
public class InetAddressEx
{
   public InetAddress address;
   public int mask;
   
   /**
    * @param address
    * @param mask
    */
   public InetAddressEx(InetAddress address, int mask)
   {
      this.address = address;
      this.mask = mask;
   }

   /**
    * @param address
    * @param mask
    */
   public InetAddressEx(InetAddress address, InetAddress mask)
   {
      this.address = address;
      this.mask = bitsInMask(mask);
   }

   /**
    * Copy constructor
    * 
    * @param src
    */
   public InetAddressEx(InetAddressEx src)
   {
      this.address = src.address;
      this.mask = src.mask;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return address.getHostAddress().replaceFirst("(^|:)(0+(:|$)){2,8}", "::") + "/" + mask;
   }
   
   /**
    * Calculate number of bits in network mask
    * 
    * @param mask
    * @return
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
    * @param bits
    * @return
    */
   public InetAddress maskFromBits()
   {
      try
      {
         byte[] bytes = address.getAddress();
         int bits = mask;
         int i = 0;
         while(bits > 8)
         {
            bytes[i++] = (byte)0xFF;
            bits -= 8;
         }
         byte b = (byte)0x80;
         while(bits > 0)
         {
            bytes[i] |= b;
            b = (byte)(b >> 1);
            bits--;
         }
         return InetAddress.getByAddress(bytes);
      }
      catch(Exception e)
      {
         return null;
      }
   }

   /* (non-Javadoc)
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

   /* (non-Javadoc)
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
