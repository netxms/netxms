/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

/**
 * Color representation in CIELAB color space
 */
public class CIELAB
{
   public float a;
   public float b;
   public float l;

   public CIELAB()
   {
      a = 0;
      b = 0;
      l = 0;
   }

   public CIELAB(float a, float b, float l)
   {
      this.a = a;
      this.b = b;
      this.l = l;
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + Float.floatToIntBits(a);
      result = prime * result + Float.floatToIntBits(b);
      result = prime * result + Float.floatToIntBits(l);
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
      CIELAB other = (CIELAB)obj;
      if (Float.floatToIntBits(a) != Float.floatToIntBits(other.a))
         return false;
      if (Float.floatToIntBits(b) != Float.floatToIntBits(other.b))
         return false;
      if (Float.floatToIntBits(l) != Float.floatToIntBits(other.l))
         return false;
      return true;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "CIELAB [a=" + a + ", b=" + b + ", l=" + l + "]";
   }
}
