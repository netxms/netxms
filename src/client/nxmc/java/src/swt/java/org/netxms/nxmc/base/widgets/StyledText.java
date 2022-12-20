/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.swt.widgets.Composite;

/**
 * Compatibility wrapper for org.eclipse.swt.custom.StyledText (so that web and desktop code can use same class)
 */
public class StyledText extends org.eclipse.swt.custom.StyledText
{
   private boolean scrollOnAppend = false;

   public StyledText(Composite parent, int style)
   {
      super(parent, style);
   }

   /**
    * Scroll content to bottom
    */
   public void scrollToBottom()
   {
      setCaretOffset(getCharCount());
      setTopIndex(getLineCount() - 1);
   }

   /**
    * Enable or disable automatic scrolling on append.
    *
    * @param enable
    */
   public void setScrollOnAppend(boolean enable)
   {
      scrollOnAppend = enable;
   }

   /**
    * @see org.eclipse.swt.custom.StyledText#append(java.lang.String)
    */
   @Override
   public void append(String string)
   {
      super.append(string);
      if (scrollOnAppend)
         scrollToBottom();
   }
}
