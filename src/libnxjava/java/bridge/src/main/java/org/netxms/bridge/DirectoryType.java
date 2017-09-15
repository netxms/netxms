/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2013 TEMPEST a.s.
 * Copyright (C) 2014-2017 Raden Solutions
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
package org.netxms.bridge;

/**
 * NetXMS directory type
 */
public enum DirectoryType
{
   BIN(0), DATA(1), ETC(2), LIB(3), SHARE(4); 
   
   private int value;

   DirectoryType(final int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }
}
