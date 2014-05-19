/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * Control group to edit Y axis range settings in chart
 */
public class YAxisRangeEditor extends Composite
{
   private Button radioAuto;
   private Button radioManual;
   private LabeledSpinner from;
   private LabeledSpinner to;
   
   /**
    * @param parent
    * @param style
    */
   public YAxisRangeEditor(Composite parent, int style)
   {
      super(parent, style);
      
      setLayout(new FillLayout());
      
      final Group group = new Group(this, SWT.NONE);
      group.setText(Messages.get().YAxisRangeEditor_Title);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      group.setLayout(layout);
      
      radioAuto = new Button(group, SWT.RADIO);
      radioAuto.setText(Messages.get().YAxisRangeEditor_Automatic);
      radioAuto.setSelection(true);
      
      from = new LabeledSpinner(group, SWT.NONE);
      from.setRange(Integer.MIN_VALUE, Integer.MAX_VALUE);
      from.setLabel(Messages.get().YAxisRangeEditor_From);
      from.setSelection(0);
      GridData gd = new GridData();
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalIndent = 15;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      from.setLayoutData(gd);

      to = new LabeledSpinner(group, SWT.NONE);
      to.setRange(Integer.MIN_VALUE, Integer.MAX_VALUE);
      to.setLabel(Messages.get().YAxisRangeEditor_To);
      to.setSelection(100);
      gd = new GridData();
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      to.setLayoutData(gd);

      radioManual = new Button(group, SWT.RADIO);
      radioManual.setText(Messages.get().YAxisRangeEditor_Manual);
      
      final SelectionAdapter s = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onModeChange();
         }
      };
      radioAuto.addSelectionListener(s);
      radioManual.addSelectionListener(s);
      
      onModeChange();
   }

   /**
    * Mode change handler
    */
   private void onModeChange()
   {
      boolean auto = radioAuto.getSelection();
      from.setEnabled(!auto);
      to.setEnabled(!auto);
   }
   
   /**
    * Set selection
    * 
    * @param auto
    * @param minY
    * @param maxY
    */
   public void setSelection(boolean auto, int minY, int maxY)
   {
      radioAuto.setSelection(auto);
      radioManual.setSelection(!auto);
      from.setSelection(minY);
      to.setSelection(maxY);
      onModeChange();
   }
   
   /**
    * @return
    */
   public boolean isAuto()
   {
      return radioAuto.getSelection();
   }
   
   /**
    * @return
    */
   public int getMinY()
   {
      return from.getSelection();
   }
   
   /**
    * @return
    */
   public int getMaxY()
   {
      return to.getSelection();
   }
}
