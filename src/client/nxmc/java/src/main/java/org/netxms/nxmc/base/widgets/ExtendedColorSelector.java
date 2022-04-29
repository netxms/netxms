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

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;

/**
 * Extended color selector - allows "unset" selection
 */
public class ExtendedColorSelector extends Composite
{
   private Group group;
   private Button colorUnset;
   private Button colorCustom;
   private ColorSelector colorSelector;

   /**
    * Create extended color selector.
    *
    * @param parent parent composite
    */
   public ExtendedColorSelector(Composite parent)
   {
      super(parent, SWT.NONE);

      setLayout(new FillLayout());

      group = new Group(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      group.setLayout(layout);

      colorUnset = new Button(group, SWT.RADIO);
      colorUnset.setText("Unset");
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      colorUnset.setLayoutData(gd);
      colorUnset.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            colorSelector.setEnabled(false);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      colorUnset.setSelection(true);

      colorCustom = new Button(group, SWT.RADIO);
      colorCustom.setText("Custom");
      colorCustom.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            colorSelector.setEnabled(true);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });

      colorSelector = new ColorSelector(group);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.grabExcessHorizontalSpace = true;
      colorSelector.getButton().setLayoutData(gd);
      colorSelector.setEnabled(false);
   }

   /**
    * Set labels. Null can be passed for any label to indicate that it should not be changed.
    *
    * @param label group label
    * @param unset text for "unset" selection
    * @param custom text for "custom" selection
    */
   public void setLabels(String label, String unset, String custom)
   {
      if (label != null)
         group.setText(label);
      if (unset != null)
         colorUnset.setText(unset);
      if (custom != null)
         colorCustom.setText(custom);
   }

   /**
    * Set color value (null to indicate "unset")
    *
    * @param rgb new color value or null
    */
   public void setColorValue(RGB rgb)
   {
      if (rgb != null)
      {
         colorUnset.setSelection(false);
         colorCustom.setSelection(true);
         colorSelector.setEnabled(true);
         colorSelector.setColorValue(rgb);
      }
      else
      {
         colorUnset.setSelection(true);
         colorCustom.setSelection(false);
         colorSelector.setEnabled(false);
      }
   }

   /**
    * Get color value.
    *
    * @return selected color value or null for "unset" selection
    */
   public RGB getColorValue()
   {
      return colorUnset.getSelection() ? null : colorSelector.getColorValue();
   }
}
