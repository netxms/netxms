/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Flat button control
 */
public class FlatButton extends Canvas
{
   private static final int MARGIN_WIDTH = 5;
   private static final int MARGIN_HEIGHT = 3;

   private String text = "";
   private boolean withBorder = false;
   private Runnable action;

   /**
    * @param parent
    * @param style
    */
   public FlatButton(Composite parent, int style)
   {
      super(parent, SWT.NONE);

      withBorder = ((style & SWT.BORDER) != 0);

      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            doPaint(e.gc);
         }
      });

      addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            if (action != null)
               action.run();
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });
   }

   /**
    * Paint control
    * 
    * @param gc graphics context
    */
   private void doPaint(GC gc)
   {
      Rectangle clientArea = getClientArea();
      gc.fillRectangle(clientArea);
      Point s = gc.textExtent(text, SWT.DRAW_MNEMONIC);
      gc.drawText(text, (clientArea.width - s.x) / 2, (clientArea.height - s.y) / 2, SWT.DRAW_MNEMONIC);
      if (withBorder)
      {
         gc.setLineWidth(2);
         gc.drawRectangle(clientArea);
      }
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if ((wHint != SWT.DEFAULT) && (hHint != SWT.DEFAULT))
         return new Point(wHint, hHint);
      Point size = WidgetHelper.getTextExtent(this, text);
      if (wHint != SWT.DEFAULT)
         size.x = wHint;
      else
         size.x += MARGIN_WIDTH * 2;
      if (hHint != SWT.DEFAULT)
         size.y = hHint;
      else
         size.y += MARGIN_HEIGHT * 2;
      return size;
   }

   /**
    * Get button's text.
    * 
    * @return button's text.
    */
   public String getText()
   {
      return text;
   }

   /**
    * Set button's text.
    * 
    * @param text new button's text.
    */
   public void setText(String text)
   {
      this.text = text;
   }

   /**
    * Get associated action.
    * 
    * @return associated action.
    */
   public Runnable getAction()
   {
      return action;
   }

   /**
    * Set associated action.
    * 
    * @param action new associated action.
    */
   public void setAction(Runnable action)
   {
      this.action = action;
   }
}
