/**
 * NetXMS Java Bridge
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
 * NeXMS log level
 */
public enum LogLevel
{
   INFO(0x0004), WARNING(0x0002), ERROR(0x0001);

   private int value;

   LogLevel(final int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }
}
