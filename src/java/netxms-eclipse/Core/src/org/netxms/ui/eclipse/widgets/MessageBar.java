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
package org.netxms.ui.eclipse.widgets;

/**
 * Message bar interface
 */
public interface MessageBar
{
   public static int INFORMATION = 0;
   public static int WARNING = 1;
   public static int ERROR = 2;

   /**
    * Show message in message bar
    * 
    * @param severity Message severity
    * @param text Message text
    */
   public void showMessage(int severity, String text);
   
   /**
    * Hide message bar
    */   
   public void hideMessage();
}
