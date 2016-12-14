/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.ui.eclipse.tools.RefreshTimer;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleEvent;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleListener;
import org.netxms.ui.eclipse.widgets.helpers.StyleRange;

/**
 * Widget for displaying html formated rich text
 */
public class StyledText extends Composite
{
   private final Browser textArea;
   private final RefreshTimer refreshTimer;
   private String textBuffer = "";
   private String largeTextBuffer = "";
   private boolean resetBuffer = true;
   private boolean setText = false;
   private Set<LineStyleListener> lineStyleListeners = new HashSet<LineStyleListener>(0); 

   /**
    * @param parent
    * @param style
    */
   public StyledText(Composite parent, int style)
   {
      super(parent, style);
      
      setLayout(new FillLayout());
      
      textArea = new Browser(this, SWT.NONE);
      textArea.setText("<pre id=\"textArea\"></pre>");

      refreshTimer = new RefreshTimer(200, this, new Runnable()
      {
        @Override
        public void run()
        {
          refresh();
        }
      });
   }
   
   /**
    * Set widget text
    * @param text
    */
   public void setText(String string)
   {
      if (!fireLineStyleListener(string))
      {
         setText = true;
         textBuffer = "";
         largeTextBuffer = "";
         append(string);
      }
   }

   /**
    * Append string to widget
    * @param string
    */
   public void append(String string)
   {
      if (!fireLineStyleListener(string))
      {
         textBuffer += string;
         largeTextBuffer += string;
         refreshTimer.execute();
      }
   }

   /** Set style for specific range of text
    * @param range
    */
   public void setStyleRange(StyleRange range)
   {
      int red = range.foreground.getRed();
      int green = range.foreground.getGreen();
      int blue = range.foreground.getBlue();

      StringBuilder sb = new StringBuilder();
      sb.append(largeTextBuffer.substring(0, (range.start)));
      sb.append("<span style=\"color:rgb(" + red + "," + green + "," + blue +
                ")\">" + largeTextBuffer.substring(range.start, (range.start + range.length)) + "</span>");
      
      largeTextBuffer = sb.toString();
      textBuffer = largeTextBuffer;
      
      setText = true;
      refreshTimer.execute();
   }
   
   /** Set style for specific range of text from event
    * @param event
    */
   public void setStyleFromEvent(LineStyleEvent e, String text)
   {
      if (e.styles == null)
         return;
      
      StringBuilder sb = new StringBuilder();
      int startOffset = 0;
      for(StyleRange range : e.styles)
      {
         int red = range.foreground.getRed();
         int green = range.foreground.getGreen();
         int blue = range.foreground.getBlue();
   
         sb.append(largeTextBuffer.substring(0, (range.start + startOffset)));
         sb.append("<span style=\"color:rgb(" + String.format("%03d", red) + "," + String.format("%03d", green) + "," + String.format("%03d", blue) +
                   ")\">" + largeTextBuffer.substring((range.start + startOffset), ((range.start + startOffset) + range.length)) + "</span>");
         
         startOffset += 47;
      }
      largeTextBuffer = sb.toString();
      textBuffer = largeTextBuffer;
      
      setText = true;
      refreshTimer.execute();
   }

   /**
    * @return Char count of text
    */
   public int getCharCount()
   {
      return largeTextBuffer.length();
   }

   /**
   * @return
   */
  public Control getControl()
   {
     return textArea;
   }

  /**
   * @return
   */
  public Composite getComposite()
   {
     return textArea;
   }

   /**
    * @param background
    */
   public void setBackground(Color background)
   {
      textArea.setBackground(background);
   }

   /**
    * Set focus on widget
    */
   @Override
   public boolean setFocus()
   {
      return textArea.setFocus();
   }
   
   /**
    * refresh widget
    */
   public void refresh()
   {
      try
      {
         String ptext = WidgetHelper.escapeText(textBuffer, true, true);
         if (setText)
         {
            resetBuffer = textArea.execute("document.getElementById(\"textArea\").innerHTML = '" + ptext + "';" + "window.scrollTo(0, document.body.scrollHeight);");
            setText = false;
         }
         else
         {
            resetBuffer = textArea.execute("document.getElementById(\"textArea\").innerHTML += '" + ptext + "';" + "window.scrollTo(0, document.body.scrollHeight);");
         }
      }
      catch (Exception e)
      {
         resetBuffer = false;
      }
      
      if (resetBuffer)
      {
         textBuffer = "";
         resetBuffer = true;
      }
      else
         refreshTimer.execute();
   }
   
   /**
    * @param listener
    */
   public void addLineStyleListener(LineStyleListener listener)
   {
      lineStyleListeners.add(listener);
   }
   
   /**
    * @param listener
    */
   public void removeLineStyleListener(LineStyleListener listener)
   {
      lineStyleListeners.remove(listener);
   }
   
   /**
    * Call all registered line style listeners
    */
   protected boolean fireLineStyleListener(String string)
   {
      if (lineStyleListeners.isEmpty())
         return false;
      
      LineStyleEvent e = new LineStyleEvent();
      e.lineText = string;
      
      for(LineStyleListener l : lineStyleListeners)
         l.lineGetStyle(e);
      
      setStyleFromEvent(e, string);
      
      return true;
   }
}
