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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Message area
 */
public class MessageArea extends Canvas implements MessageAreaHolder
{
   public static final int INFO = 0;
   public static final int SUCCESS = 1;
   public static final int WARNING = 2;
   public static final int ERROR = 3;

   private static final int MAX_ROWS = 3;
   private static final int MARGIN_WIDTH = 4;
   private static final int MARGIN_HEIGHT = 4;
   private static final int MESSAGE_SPACING = 3;
   private static final int TEXT_MARGIN_WIDTH = 5;
   private static final int TEXT_MARGIN_HEIGHT = 5;

   private int nextMessageId = 1;
   private List<Message> messages = new ArrayList<>(0);

   /**
    * Create message area.
    *
    * @param parent parent composite
    * @param style style bits
    */
   public MessageArea(Composite parent, int style)
   {
      super(parent, style);
      setBackground(ThemeEngine.getBackgroundColor("MessageArea"));
      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            doPaint(e.gc);
         }
      });
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#addMessage(int, java.lang.String)
    */
   @Override
   public int addMessage(int level, String text)
   {
      Message m = new Message(nextMessageId++, level, text);
      messages.add(0, m);
      getParent().layout(true, true);
      return m.id;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#deleteMessage(int)
    */
   @Override
   public void deleteMessage(int id)
   {
      for(int i = 0; i < messages.size(); i++)
         if (messages.get(i).id == id)
         {
            messages.remove(i);
            getParent().layout(true, true);
            break;
         }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.MessageAreaHolder#clearMessages()
    */
   @Override
   public void clearMessages()
   {
      if (messages.isEmpty())
         return;
      messages.clear();
      getParent().layout(true, true);
   }

   /**
    * Do painting of message area
    *
    * @param gc graphics context
    */
   private void doPaint(GC gc)
   {
      if (messages.isEmpty())
         return;

      Color textColor = ThemeEngine.getForegroundColor("MessageArea");
      Rectangle clientArea = getClientArea();
      int x = clientArea.x + MARGIN_WIDTH;
      int y = clientArea.y + MARGIN_HEIGHT;
      int width = clientArea.width - MARGIN_WIDTH * 2;
      int height = calculateRowHeight();
      int rowCount = Math.min(messages.size(), MAX_ROWS);
      for(int i = 0; i < rowCount; i++)
      {
         Message m = messages.get(i);
         gc.setBackground(m.getBackgroundColor());
         gc.fillRoundRectangle(x, y, width, height, 4, 4);
         gc.setForeground(m.getBorderColor());
         gc.drawRoundRectangle(x, y, width, height, 4, 4);
         gc.setForeground(textColor);
         gc.drawText(m.text, x + TEXT_MARGIN_WIDTH + 1, y + TEXT_MARGIN_HEIGHT + 1, true);
         y += height + MESSAGE_SPACING;
      }
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      int rowCount = Math.min(messages.size(), MAX_ROWS);
      if (rowCount == 0)
         return new Point(0, 0);
      int rowHeight = calculateRowHeight();
      int optimalHeight = rowHeight * rowCount + MESSAGE_SPACING * (rowCount - 1) + MARGIN_HEIGHT * 2;
      if ((hHint == SWT.DEFAULT) || (optimalHeight <= hHint))
         return new Point((wHint == SWT.DEFAULT) ? 600 : wHint, optimalHeight);
      return new Point((wHint == SWT.DEFAULT) ? 600 : wHint, hHint);
   }

   /**
    * Calculate row height in pixels.
    *
    * @return row height in pixels
    */
   private int calculateRowHeight()
   {
      return WidgetHelper.getTextExtent(this, "X").y + TEXT_MARGIN_HEIGHT * 2 + 2;
   }

   /**
    * Message to display
    */
   private static class Message
   {
      int id;
      int level;
      String text;

      Message(int id, int level, String text)
      {
         this.id = id;
         this.level = level;
         this.text = text;
      }

      Color getBackgroundColor()
      {
         switch(level)
         {
            case SUCCESS:
               return ThemeEngine.getBackgroundColor("MessageArea.Success");
            case WARNING:
               return ThemeEngine.getBackgroundColor("MessageArea.Warning");
            case ERROR:
               return ThemeEngine.getBackgroundColor("MessageArea.Error");
            default:
               return ThemeEngine.getBackgroundColor("MessageArea.Info");
         }
      }

      Color getBorderColor()
      {
         switch(level)
         {
            case SUCCESS:
               return ThemeEngine.getForegroundColor("MessageArea.Success");
            case WARNING:
               return ThemeEngine.getForegroundColor("MessageArea.Warning");
            case ERROR:
               return ThemeEngine.getForegroundColor("MessageArea.Error");
            default:
               return ThemeEngine.getForegroundColor("MessageArea.Info");
         }
      }
   }
}
