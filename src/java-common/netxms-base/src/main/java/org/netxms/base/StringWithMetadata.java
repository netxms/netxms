/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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

/**
 * String object with additional metadata
 */
public class StringWithMetadata
{
   private String string;
   private boolean sensitive;

   /**
    * Create empty string
    */
   public StringWithMetadata()
   {
      string = "";
      sensitive = false;
   }

   /**
    * Create ordinary string
    * 
    * @param string string object
    */
   public StringWithMetadata(String string)
   {
      this.string = (string != null) ? string : "";
      this.sensitive = false;
   }
   
   /**
    * Create string with "sensitive" attribute
    * 
    * @param string string object
    * @param sensitive sensitivity flag
    */
   public StringWithMetadata(String string, boolean sensitive)
   {
      this.string = (string != null) ? string : "";
      this.sensitive = sensitive;
   }
   
   /**
    * Create copy of other string
    * 
    * @param src source string
    */
   public StringWithMetadata(StringWithMetadata src)
   {
      this.string = src.string;
      this.sensitive = src.sensitive;
   }

   /**
    * @return the string
    */
   public String getString()
   {
      return string;
   }

   /**
    * @return the sensitive
    */
   public boolean isSensitive()
   {
      return sensitive;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return string;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      return string.hashCode();
   }

   /* (non-Javadoc)
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      return string.equals(obj);
   }
}
