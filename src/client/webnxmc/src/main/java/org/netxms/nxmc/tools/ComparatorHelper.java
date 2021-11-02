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
package org.netxms.nxmc.tools;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;

/**
 * Helper class for comparators
 */
public class ComparatorHelper
{
	public static int compareInetAddresses(InetAddress a1, InetAddress a2)
	{
	   if (a1 == null)
	      return (a2 == null) ? 0 : -1;

	   if (a2 == null)
         return 1;
	   
	   if ((a1 instanceof Inet4Address) && (a2 instanceof Inet6Address))
	      return -1;

	   if ((a1 instanceof Inet6Address) && (a2 instanceof Inet4Address))
         return 1;
	   
		byte[] b1 = a1.getAddress(); 
		byte[] b2 = a2.getAddress();
		int length = Math.min(b1.length, b2.length);
		for(int i = 0; i < length; i++)
		{
			int n1 = (int)b1[i] & 0xFF; 
			int n2 = (int)b2[i] & 0xFF; 
			if (n1 < n2)
				return -1;
			if (n1 > n2)
				return 1;
		}
		return b1.length - b2.length;
	}

   /**
    * Return char at specific position
    * 
    * @param s String
    * @param i position
    * @return char at position
    */
   private static char charAt(String s, int i)
   {
      return i >= s.length() ? 0 : s.charAt(i);
   }

   /**
    * Check if char is digit or not
    * 
    * @param a String a
    * @param b String b
    * @return result
    */
   private static int compareRight(String a, String b)
   {
      int bias = 0, i = 0, n = 0;

      while(true)
      {
         char ca = charAt(a, i);
         char cb = charAt(b, n);

         if (!Character.isDigit(ca) && !Character.isDigit(cb))
            return bias;
         if (!Character.isDigit(ca))
            return -1;
         if (!Character.isDigit(cb))
            return +1;
         if (ca == 0 && cb == 0)
            return bias;

         if (bias == 0)
         {
            if (ca < cb)
               bias = -1;
            else if (ca > cb)
               bias = +1;
         }
         i++;
         n++;
      }
   }

   /**
    * Compare two strings using natural sorting order
    * 
    * @param a String a
    * @param b String b
    * @return result
    */
   public static int compareStringsNatural(String a, String b)
   {
      int i = 0, n = 0;
      int j = 0, c = 0;
      char ca, cb;

      while(true)
      {
         j = c = 0;

         ca = charAt(a, i);
         cb = charAt(b, n);

         while(Character.isSpaceChar(ca) || ca == '0')
         {
            if (ca == '0')
               j++;
            else
               j = 0;

            ca = charAt(a, ++i);
         }

         while(Character.isSpaceChar(cb) || cb == '0')
         {
            if (cb == '0')
               c++;
            else
               c = 0;

            cb = charAt(b, ++n);
         }

         if (Character.isDigit(ca) && Character.isDigit(cb))
         {
            int bias = compareRight(a.substring(i), b.substring(n));
            if (bias != 0)
               return bias;
         }

         if (ca == 0 && cb == 0)
            return j - c;
         if (ca < cb)
            return -1;
         if (ca > cb)
            return +1;

         ++i;
         ++n;
      }
   }
}
