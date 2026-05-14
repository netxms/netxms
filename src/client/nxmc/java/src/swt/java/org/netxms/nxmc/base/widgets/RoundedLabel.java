/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Rounded label - rounded rectangle with text inside.
 */
public class RoundedLabel extends Canvas
{
   private static final RGB WHITE = new RGB(255, 255, 255);

   private Label label;
   private Color labelDarkColor;
   private Color labelLightColor;
   private ColorCache colorCache;
   private Color fillColor;
   private Color borderColor;
   private boolean blendMode = true;

   /**
    * Create new rounded label
    *
    * @param parent parent composite
    */
   public RoundedLabel(Composite parent)
   {
      super(parent, SWT.NONE);

      labelDarkColor = getDisplay().getSystemColor(SWT.COLOR_BLACK);
      labelLightColor = getDisplay().getSystemColor(SWT.COLOR_GRAY);
      colorCache = new ColorCache(this);
      fillColor = parent.getBackground();
      borderColor = null;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 3;
      layout.marginWidth = 8;
      setLayout(layout);

      setBackground(parent.getBackground());

      label = new Label(this, SWT.CENTER);
      label.setBackground(getBackground());
      label.setForeground(ColorConverter.isDarkColor(label.getBackground()) ? labelLightColor : labelDarkColor);
      label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));

      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            e.gc.setAntialias(SWT.ON);
            Rectangle r = getClientArea();
            e.gc.setBackground(fillColor);
            e.gc.fillRoundRectangle(0, 0, r.width - 1, r.height - 1, 10, 10);
            if (borderColor != null)
            {
               e.gc.setForeground(borderColor);
               e.gc.setLineWidth(1);
               e.gc.drawRoundRectangle(0, 0, r.width - 1, r.height - 1, 10, 10);
            }
         }
      });
   }

   /**
    * @return the blendMode
    */
   public boolean isBlendMode()
   {
      return blendMode;
   }

   /**
    * @param blendMode the blendMode to set
    */
   public void setBlendMode(boolean blendMode)
   {
      this.blendMode = blendMode;
   }

   /**
    * Set label background color. If blend mode is on then widget will use a light tint of this color as fill and the color itself
    * as border and text color.
    *
    * @param color background color or accent color in blend mode
    */
   public void setLabelBackground(Color color)
   {
      if (color != null)
      {
         if (blendMode)
         {
            RGB accent = color.getRGB();
            fillColor = colorCache.create(ColorConverter.blend(accent, WHITE, 20));
            borderColor = colorCache.create(accent);
            RGB textRgb = ColorConverter.isDarkColor(accent) ? accent : ColorConverter.blend(accent, new RGB(0, 0, 0), 60);
            label.setForeground(colorCache.create(textRgb));
         }
         else
         {
            fillColor = color;
            borderColor = null;
            label.setForeground(ColorConverter.isDarkColor(fillColor) ? labelLightColor : labelDarkColor);
         }
      }
      else
      {
         fillColor = getParent().getBackground();
         borderColor = null;
         label.setForeground(ColorConverter.isDarkColor(fillColor) ? labelLightColor : labelDarkColor);
      }
      label.setBackground(fillColor);
      redraw();
   }

   /**
    * Set label foreground colors. Can pass null as any color to reset to default value.
    *
    * @param darkColor dark foreground color (to be used on light background)
    * @param lightColor light foreground color (to be used on dark background)
    */
   public void setLabelForeground(Color darkColor, Color lightColor)
   {
      labelDarkColor = (darkColor != null) ? darkColor : getDisplay().getSystemColor(SWT.COLOR_BLACK);
      labelLightColor = (lightColor != null) ? lightColor : getDisplay().getSystemColor(SWT.COLOR_GRAY);
      label.setForeground(ColorConverter.isDarkColor(label.getBackground()) ? labelLightColor : labelDarkColor);
   }

   /**
    * @see org.eclipse.swt.widgets.Canvas#setFont(org.eclipse.swt.graphics.Font)
    */
   @Override
   public void setFont(Font font)
   {
      label.setFont(font);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setToolTipText(java.lang.String)
    */
   @Override
   public void setToolTipText(String string)
   {
      super.setToolTipText(string);
      label.setToolTipText(string);
   }

   /**
    * Set label text.
    *
    * @param text label text
    */
   public void setText(String text)
   {
      label.setText(text);
   }
}
