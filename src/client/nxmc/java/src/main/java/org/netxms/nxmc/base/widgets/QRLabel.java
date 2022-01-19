/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import io.nayuki.qrcodegen.QrCode;

/**
 * Label that displays QR code
 */
public class QRLabel extends Canvas
{
   private String text = "";

   /**
    * @param parent
    * @param style
    */
   public QRLabel(Composite parent, int style)
   {
      super(parent, style);
      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            drawQRCode(e.gc);
         }
      });
   }

   /**
    * Draw QR code
    *
    * @param gc GC to draw on
    */
   private void drawQRCode(GC gc)
   {
      Rectangle rect = getClientArea();
      int offsetWidth = 0;
      int offsetHeight = 0;
      if (rect.width > rect.height)
      {
         offsetWidth = (rect.width - rect.height) / 2;
         rect.width = rect.height;
      }
      else if (rect.width < rect.height)
      {
         offsetHeight = (rect.height - rect.width) / 2;
         rect.height = rect.width;
      }

      QrCode qr = QrCode.encodeText(text, QrCode.Ecc.MEDIUM);
      float scale = (float)rect.width / (float)qr.size;
      for(int y = 0; y < rect.height; y++)
      {
         for(int x = 0; x < rect.width; x++)
         {
            if (qr.getModule((int)((float)x / scale), (int)((float)y / scale)))
               gc.drawPoint(x + offsetWidth, y + offsetHeight);
         }
      }
   }

   /**
    * Get text from widget.
    * 
    * @return current text
    */
   public String getText()
   {
      return text;
   }

   /**
    * Set text to display.
    *
    * @param text text to display
    */
   public void setText(String text)
   {
      this.text = text;
      redraw();
   }
}
