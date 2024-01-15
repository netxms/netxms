/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Custom text control that can display image besides text
 */
public class CText extends Composite
{
   private Label imageControl;
   private Text textControl;

   /**
    * Create custom text control.
    *
    * @param parent parent composite
    * @param style style of the control
    */
   public CText(Composite parent, int style)
   {
      super(parent, style & ~SWT.READ_ONLY);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 3;
      layout.marginHeight = 3;
      setLayout(layout);

      imageControl = new Label(this, SWT.CENTER);
      imageControl.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, true));
      imageControl.setVisible(false);
      ((GridData)imageControl.getLayoutData()).exclude = true;

      textControl = new Text(this, style & SWT.READ_ONLY);
      textControl.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));

      imageControl.setBackground(textControl.getBackground());
      setBackground(textControl.getBackground());
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      return textControl.setFocus();
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setToolTipText(java.lang.String)
    */
   @Override
   public void setToolTipText(String string)
   {
      super.setToolTipText(string);
      textControl.setToolTipText(string);
      imageControl.setToolTipText(string);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setMenu(org.eclipse.swt.widgets.Menu)
    */
   @Override
   public void setMenu(Menu menu)
   {
      super.setMenu(menu);
      textControl.setMenu(menu);
      imageControl.setMenu(menu);
   }

   /**
    * Set image to display
    *
    * @param image new image
    */
   public void setImage(Image image)
   {
      imageControl.setImage(image);
      if (image != null)
      {
         imageControl.setVisible(true);
         ((GridData)imageControl.getLayoutData()).exclude = false;
      }
      else
      {
         imageControl.setVisible(false);
         ((GridData)imageControl.getLayoutData()).exclude = true;
      }
      layout(true, true);
   }

   /**
    * Get image.
    *
    * @return image
    */
   public Image getImage()
   {
      return imageControl.getImage();
   }

   /**
    * Set text.
    *
    * @param text new text
    */
   public void setText(String text)
   {
      textControl.setText(text);
   }

   /**
    * Get text.
    *
    * @return current text
    */
   public String getText()
   {
      return textControl.getText();
   }

   /**
    * Get text control.
    *
    * @return text control
    */
   public Text getTextControl()
   {
      return textControl;
   }

   /**
    * Enable/disable text editing.
    *
    * @param editable true to enable edit
    */
   public void setEditable(boolean editable)
   {
      textControl.setEditable(editable);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      imageControl.setBackground(color);
      textControl.setBackground(color);
   }
}
