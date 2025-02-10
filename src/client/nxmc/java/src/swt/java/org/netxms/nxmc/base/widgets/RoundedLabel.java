/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Rounded label - rounded rectangle with text inside.
 */
public class RoundedLabel extends Canvas
{
   private Label label;
   private Color labelDarkColor;
   private Color labelLightColor;

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

      GridLayout layout = new GridLayout();
      layout.marginHeight = 4;
      layout.marginWidth = 4;
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
            e.gc.setBackground(label.getBackground());
            Rectangle r = getClientArea();
            e.gc.fillRoundRectangle(0, 0, r.width - 1, r.height - 1, 8, 8);
         }
      });
   }

   /**
    * Set label background color.
    *
    * @param color label background color
    */
   public void setLabelBackground(Color color)
   {
      label.setBackground((color != null) ? color : getParent().getBackground());
      label.setForeground(ColorConverter.isDarkColor((color != null) ? color : label.getBackground()) ? labelLightColor : labelDarkColor);
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
