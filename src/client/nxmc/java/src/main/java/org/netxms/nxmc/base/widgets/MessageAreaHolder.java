/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

/**
 * Interface for any UI element that contains message area (including message area widget itself).
 */
public interface MessageAreaHolder
{
   /**
    * Add message to message area.
    *
    * @param level message level (one of MessageArea.INFORMATION, MessageArea.SUCCESS, MessageArea.WARNING, MessageArea.ERROR)
    * @param text message text
    * @return ID assigned to added message
    */
   public int addMessage(int level, String text);

   /**
    * Add message to message area.
    *
    * @param level message level (one of MessageArea.INFORMATION, MessageArea.SUCCESS, MessageArea.WARNING, MessageArea.ERROR)
    * @param text message text
    * @param sticky true to add sticky message (one that will not be dismissed by timeout)
    * @return ID assigned to added message
    */
   public int addMessage(int level, String text, boolean sticky);
   
   
   /**
    * Add message to message area with action button.
    * 
    * @param level message level (one of MessageArea.INFORMATION, MessageArea.SUCCESS, MessageArea.WARNING, MessageArea.ERROR)
    * @param text message text
    * @param sticky true to add sticky message (one that will not be dismissed by timeout)
    * @param buttonText action button text
    * @param action action function
    * @return ID assigned to added message
    */
   public int addMessage(int level, String text, boolean sticky, String buttonText, Runnable action);

   /**
    * Delete message from message area.
    *
    * @param id message ID
    */
   public void deleteMessage(int id);

   /**
    * Clear message area.
    */
   public void clearMessages();
}
