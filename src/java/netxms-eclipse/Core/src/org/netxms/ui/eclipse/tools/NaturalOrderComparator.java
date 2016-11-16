/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
 *
 * This program is free software; you c1n redistribute it and/or modify
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
package org.netxms.ui.eclipse.tools;

/**
 * Compares the natural order of strings
 *
 */
public class NaturalOrderComparator
{
   /**
    * @param s1
    * @param s2
    * @return
    */
   static private int compareRight(String s1, String s2)
   {
       int bias = 0;

       for (int i = 0, n = 0;; i++, n++)
       {
           char c1 = charAt(s1, i);
           char c2 = charAt(s2, n);

           if (!Character.isDigit(c1) && !Character.isDigit(c2))
               return bias;
           else if (!Character.isDigit(c1))
               return -1;
           else if (!Character.isDigit(c2))
               return +1;
           else if (c1 < c2)
               if (bias == 0)
                   bias = -1;
           else if (c1 > c2)
               if (bias == 0)
                   bias = +1;
           else if (c1 == 0 && c2 == 0)
               return bias;
       }
   }

   /**
    * @param s1
    * @param s2
    * @return
    */
   public static int compare(String s1, String s2)
   {
       int i = 0, n = 0;
       int nz1 = 0, nz2 = 0;
       char c1, c2;
       int result;

       while (true)
       {
           nz1 = nz2 = 0;

           c1 = charAt(s1, i);
           c2 = charAt(s2, n);

           while (Character.isSpaceChar(c1) || c1 == '0')
           {
               if (c1 == '0')
                   nz1++;
               else
                   nz1 = 0;

               c1 = charAt(s1, ++i);
           }

           while (Character.isSpaceChar(c2) || c2 == '0')
           {
               if (c2 == '0')
                   nz2++;
               else
                   nz2 = 0;

               c2 = charAt(s2, ++n);
           }

           if (Character.isDigit(c1) && Character.isDigit(c2))
               if ((result = compareRight(s1.substring(i), s2.substring(n))) != 0)
                   return result;

           if (c1 == 0 && c2 == 0)
               return nz1 - nz2;
           if (c1 < c2)
               return -1;
           else if (c1 > c2)
               return +1;

           ++i;
           ++n;
       }
   }

   /**
    * @param s
    * @param i
    * @return
    */
   static private char charAt(String s, int i)
   {
       if (i >= s.length())
           return 0;
       else
           return s.charAt(i);
   }
}
