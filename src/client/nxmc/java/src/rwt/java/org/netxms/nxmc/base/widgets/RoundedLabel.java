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

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Rounded label - rounded rectangle with text inside.
 */
public class RoundedLabel extends Composite
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
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      setBackground(parent.getBackground());

      label = new Label(this, SWT.CENTER);
      label.setData(RWT.CUSTOM_VARIANT, "RoundedLabel");
      label.setBackground(getBackground());
      label.setForeground(ColorConverter.isDarkColor(label.getBackground()) ? labelLightColor : labelDarkColor);
      label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));
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
