/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2016 Raden Solutions
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
package org.netxms.agent;

/**
 * Plugin initialization exception
 */
public class PluginInitException extends Exception
{
   private static final long serialVersionUID = 1L;

   public PluginInitException(String text, Throwable e)
   {
      super(text, e);
   }

   public PluginInitException(String text)
   {
      super(text);
   }
}
