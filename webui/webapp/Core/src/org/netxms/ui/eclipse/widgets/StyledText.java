/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.tools.RefreshTimer;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleEvent;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleListener;
import org.netxms.ui.eclipse.widgets.helpers.StyleRange;

/**
 * Read-only implementation of SWT StyledText control with compatible interface 
 */
public class StyledText extends Composite
{
   private final Browser textArea;
   private final RefreshTimer refreshTimer;
   private StringBuilder content = new StringBuilder();
   private List<Integer> lineOffsets = new ArrayList<>();
   private Map<Integer, StyleRange> styleRanges = new TreeMap<Integer, StyleRange>(); 
   private int writePosition = 0;
   private boolean refreshContent = false;
   private boolean refreshInProgress = false;
   private Set<LineStyleListener> lineStyleListeners = new HashSet<LineStyleListener>(0);
   private boolean scrollOnAppend = false;
   private boolean forceScroll = false;

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

      refreshTimer = new RefreshTimer(200, this, new Runnable() {
         @Override
         public void run()
         {
            refresh();
         }
      });
   }

   /**
    * Set widget text
    *
    * @param text new text
    */
   public void setText(String text)
   {
      checkWidget();
      styleRanges.clear();
      content = new StringBuilder(text);
      writePosition = 0;
      refreshContent = true;
      fireLineStyleListeners(0);
      refreshTimer.execute();
   }

   /**
    * Append string to widget
    * @param text
    */
   public void append(String text)
   {
      checkWidget();
      int pos = content.length();
      content.append(text);
      fireLineStyleListeners(pos);
      refreshTimer.execute();
   }

   /**
    * Replace given text range with new content.
    * 
    * @param start start offset
    * @param length length of text to be replaced
    * @param text new content
    */
   public void replaceTextRange(int start, int length, String text)
   {
      if (start >= content.length())
         return;

      if (start + length > content.length())
         length = content.length() - start;

      content.replace(start, start + length, text);
      styleRanges.clear();
      lineOffsets.clear();
      writePosition = 0;
      refreshContent = true;
      fireLineStyleListeners(0);
      refreshTimer.execute();
   }

   /** Set style for specific range of text
    * @param range
    */
   public void setStyleRange(StyleRange range)
   {
      checkWidget();
      if ((range.start < 0) || (range.start >= content.length()) || (range.length == 0))
         return;
      styleRanges.put(range.start, range);
      if (range.start <= writePosition)
         refreshContent = true;
      refreshTimer.execute();
   }

   /**
    * @return Char count of text
    */
   public int getCharCount()
   {
      return content.length();
   }

   /**
   * @return
   */
   public Control getControl()
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
    * @return
    */
   public boolean isScrollOnAppend()
   {
      return scrollOnAppend;
   }

   /**
    * @param scrollOnAppend
    */
   public void setScrollOnAppend(boolean scrollOnAppend)
   {
      this.scrollOnAppend = scrollOnAppend;
   }

   /** 
    * Apply style range
    * 
    * @param range
    * @param text
    */
   private String applyStyleRange(StyleRange range, String text)
   {
      if (range.hidden)
         return "";

      StringBuilder sb = new StringBuilder();
      sb.append("<span style=\"");
      if (range.foreground != null)
      {
         sb.append("color:rgb(");
         sb.append(range.foreground.getRed());
         sb.append(',');
         sb.append(range.foreground.getGreen());
         sb.append(',');
         sb.append(range.foreground.getBlue());
         sb.append(");");
      }
      if (range.background != null)
      {
         sb.append("background-color:rgb(");
         sb.append(range.background.getRed());
         sb.append(',');
         sb.append(range.background.getGreen());
         sb.append(',');
         sb.append(range.background.getBlue());
         sb.append(");");
      }
      if (range.fontStyle == SWT.BOLD)
      {
         sb.append("font:bold;");
      }
      if (range.underline)
      {
         sb.append("text-decoration: underline;");
      }
      if (range.strikeout)
      {
         sb.append("text-decoration: line-through;");
      }
      sb.append("\">");
      sb.append(text);
      sb.append("</span>");
      return sb.toString();
   }
   
   /**
    * Apply style ranges starting at given position
    * 
    * @param startPos
    * @return
    */
   private String applyStyleRanges(int startPos)
   {
      StringBuilder result = new StringBuilder();
      int currPos = startPos;
      for(StyleRange r : styleRanges.values())
      {
         if (r.start + r.length <= currPos)
            continue;

         if (currPos < r.start)
            result.append(WidgetHelper.escapeText(content.substring(currPos, r.start), true, true));
         result.append(applyStyleRange(r, WidgetHelper.escapeText(content.substring(r.start, r.start + r.length), true, true)));
         currPos = r.start + r.length;
      }
      result.append(WidgetHelper.escapeText(content.substring(currPos), true, true));
      return result.toString();
   }

   /**
    * refresh widget
    */
   private void refresh()
   {
      // Call to textArea.execute may call readAndDispatch() on Display
      // potentially causing recursive call of this method. In this case reschedule
      // refresh and return immediately.
      if (refreshInProgress)
      {
         refreshTimer.execute();
         return;
      }

      refreshInProgress = true;
      boolean success;
      try
      {
         StringBuilder js = new StringBuilder("document.getElementById(\"textArea\").innerHTML");
         if (refreshContent)
         {
            js.append("='");
            js.append(applyStyleRanges(0));
         }
         else
         {
            js.append("+='");
            js.append(applyStyleRanges(writePosition));
         }
         js.append("';");
         if (scrollOnAppend || forceScroll)
            js.append("window.scrollTo(0, document.body.scrollHeight);");
         int contentLength = content.length();  // content can be updated by calls to append() while script is executing
         success = textArea.execute(js.toString());
         if (success)
         {
            writePosition = contentLength;
            refreshContent = false;
            forceScroll = false;
         }
      }
      catch (Exception e)
      {
         Activator.logError("Exception during StyledText content update", e);
         success = false;
      }
      refreshInProgress = false;

      if (!success)
         refreshTimer.execute();
   }

   /**
    * Scroll content to bottom
    */
   public void scrollToBottom()
   {
      // Call to textArea.execute may call readAndDispatch() on Display
      // potentially causing recursive call of this method. In this case reschedule
      // refresh and return immediately.
      if (refreshInProgress)
      {
         forceScroll = true;
         refreshTimer.execute();
         return;
      }

      refreshInProgress = true;
      boolean success;
      try
      {
         textArea.execute("window.scrollTo(0, document.body.scrollHeight);");
         success = true;
         forceScroll = false;
      }
      catch(Exception e)
      {
         Activator.logError("Exception during StyledText forced scroll", e);
         success = false;
      }
      refreshInProgress = false;

      if (!success)
      {
         forceScroll = true;
         refreshTimer.execute();
      }
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
    * @param line
    * @param lineStartPos
    */
   private void styleLine(String line, int lineStartPos)
   {
      LineStyleEvent e = new LineStyleEvent();
      e.lineText = line;
      for(LineStyleListener l : lineStyleListeners)
         l.lineGetStyle(e);
      if (e.styles != null)
      {
         for(StyleRange r : e.styles)
         {
            r.start += lineStartPos;
            styleRanges.put(r.start, r);
         }
      }
   }

   /**
    * Update line offsets starting at given position in text
    *
    * @param startPos start position
    */
   protected void updateLineOffsets(int startPos)
   {
      if ((startPos == 0) || (content.charAt(startPos - 1) == '\n'))
         lineOffsets.add(startPos);

      int lineStartPos = startPos;
      char[] text = content.substring(startPos).toCharArray();
      for(int i = 0; i < text.length; i++)
      {
         if (text[i] == '\n')
         {
            lineStartPos = startPos + i + 1;
            lineOffsets.add(lineStartPos);
         }
      }
   }

   /**
    * Call all registered line style listeners
    */
   protected void fireLineStyleListeners(int startPos)
   {
      if (lineStyleListeners.isEmpty())
      {
         updateLineOffsets(startPos);
         return;
      }

      if ((startPos == 0) || (content.charAt(startPos - 1) == '\n'))
         lineOffsets.add(startPos);

      StringBuilder line = new StringBuilder();
      int lineStartPos = startPos;
      char[] text = content.substring(startPos).toCharArray();
      for(int i = 0; i < text.length; i++)
      {
         if (text[i] == '\n')
         {
            styleLine(line.toString(), lineStartPos);
            lineStartPos = startPos + i + 1;
            lineOffsets.add(lineStartPos);
            line = new StringBuilder();
         }
         else if (text[i] != '\r')
         {
            line.append(text[i]);
         }
      }

      if (line.length() > 0)
      {
         styleLine(line.toString(), lineStartPos);
      }
   }

   /**
    * Get widget text.
    *
    * @return text from widget
    */
   public String getText()
   {
      return content.toString();
   }

   /**
    * Get number of lines
    *
    * @return number of lines
    */
   public int getLineCount()
   {
      return lineOffsets.size();
   }

   /**
    * Get offset of given line
    *
    * @param line
    * @return
    */
   public int getOffsetAtLine(int line)
   {
      return ((line >= 0) && (line < lineOffsets.size())) ? lineOffsets.get(line) : -1;
   }

   /**
    * Compatibility method, does nothing
    *
    * @param editable
    */
   public void setEditable(boolean editable)
   {
   }
}
