/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.base.views.helpers;

/**
 * Navigation history provided by navigation view.
 */
public class NavigationHistory
{
   private static final int HISTORY_SIZE = 64;

   private Object[] history = new Object[HISTORY_SIZE];
   private int size = 0;
   private int position = -1;
   private int bufferPosition = 0;
   private boolean locked = false;

   /**
    * Add new item to history. Item will be added after current one and history position will be advanced. Any items in history
    * after current position will be removed (so <code>forward()</code> call will return null).
    * 
    * @param selection
    */
   public void add(Object selection)
   {
      if (locked)
         return;

      history[bufferPosition++] = selection;
      if (bufferPosition == HISTORY_SIZE)
         bufferPosition = 0;
      if (position < HISTORY_SIZE - 1)
         position++;
      size = position + 1;
   }

   /**
    * Go back in history. Will shift history position backwards and return new current element.
    *
    * @return new current element or null if already at the beginning of the history
    */
   public Object back()
   {
      if (position < 1)
         return null;
      position--;
      bufferPosition--;
      if (bufferPosition < 0)
         bufferPosition = HISTORY_SIZE - 1;
      return history[bufferPosition > 0 ? bufferPosition - 1 : HISTORY_SIZE - 1];
   }

   /**
    * Go forward in history. Will shift history position forward and return new current element.
    *
    * @return new current element or null if already at the end of the history
    */
   public Object forward()
   {
      if (position >= size - 1)
         return null;
      position++;
      Object e = history[bufferPosition++];
      if (bufferPosition == HISTORY_SIZE)
         bufferPosition = 0;
      return e;
   }

   /**
    * Check if history can be moved backward.
    * 
    * @return true if history can be moved backward
    */
   public boolean canGoBackward()
   {
      return position > 0;
   }

   /**
    * Check if history can be moved forward.
    * 
    * @return true if history can be moved forward
    */
   public boolean canGoForward()
   {
      return position < size - 1;
   }

   /**
    * Clear history
    */
   public void clear()
   {
      size = 0;
      position = -1;
      bufferPosition = 0;
   }

   /**
    * Lock for updates
    */
   public void lock()
   {
      locked = true;
   }

   /**
    * Unlock for updates
    */
   public void unlock()
   {
      locked = false;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NavigationHistory [size=" + size + ", position=" + position + ", bufferPosition=" + bufferPosition + "]";
   }
}
