/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Compatibility wrapper for org.eclipse.swt.custom.StyledText (so that web and desktop code can use same class)
 */
public class StyledText extends org.eclipse.swt.custom.StyledText
{
   private static final Logger logger = LoggerFactory.getLogger(StyledText.class);

   private boolean scrollOnAppend = false;
   private LineStyler lineStyler = null;
   private LineStyleListener lineStyleListener = null;

   /**
    * Create new styled text control.
    *
    * @param parent parent composite
    * @param style control style
    */
   public StyledText(Composite parent, int style)
   {
      super(parent, style);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (lineStyler != null)
               lineStyler.dispose();
         }
      });
   }

   /**
    * Replace content (including all styling) with one from given source styled text control
    * 
    * @param src source styled text control
    */
   public void replaceContent(StyledText src)
   {
      setText(src.getText());
      setStyleRanges(src.getStyleRanges());
      setCaretOffset(src.getCaretOffset());
      setTopIndex(src.getTopIndex());
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

   /**
    * @return the lineStyler
    */
   public LineStyler getLineStyler()
   {
      return lineStyler;
   }

   /**
    * @param lineStyler the lineStyler to set
    */
   public void setLineStyler(LineStyler lineStyler)
   {
      if (this.lineStyler == lineStyler)
         return;

      if (this.lineStyler != null)
         this.lineStyler.dispose();

      this.lineStyler = lineStyler;
      if (lineStyler != null)
      {
         if (lineStyleListener == null)
         {
            lineStyleListener = new LineStyleListener() {
               @Override
               public void lineGetStyle(LineStyleEvent event)
               {
                  try
                  {
                     event.styles = lineStyler.styleLine(event.lineText);
                     if (event.styles != null)
                     {
                        for(StyleRange r : event.styles)
                           r.start += event.lineOffset;
                     }
                  }
                  catch(Exception e)
                  {
                     logger.error("Exception in line style listener", e);
                  }
               }
            };
            addLineStyleListener(lineStyleListener);
         }
      }
      else
      {
         if (lineStyleListener != null)
         {
            removeLineStyleListener(lineStyleListener);
            lineStyleListener = null;
         }
      }
   }
}
