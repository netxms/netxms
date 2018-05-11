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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Control group to edit Y axis range settings in chart
 */
public class YAxisRangeEditor extends Composite
{
   private Button radioAuto;
   private Button radioManual;
   private Button checkYBase;
   private LabeledText from;
   private LabeledText to;
   private Color errorBackground;
   
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
      
      checkYBase = new Button(group, SWT.CHECK);
      checkYBase.setText("Set Y base to min value");
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      checkYBase.setLayoutData(gd);

      radioManual = new Button(group, SWT.RADIO);
      radioManual.setText(Messages.get().YAxisRangeEditor_Manual);
      
      from = new LabeledText(group, SWT.NONE);
      from.setLabel(Messages.get().YAxisRangeEditor_From);
      from.setText("0");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      from.setLayoutData(gd);

      to = new LabeledText(group, SWT.NONE);
      to.setLabel(Messages.get().YAxisRangeEditor_To);
      to.setText("100");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      to.setLayoutData(gd);
      
      final SelectionAdapter s = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onModeChange();
         }
      };
      radioAuto.addSelectionListener(s);
      radioManual.addSelectionListener(s);
      
      errorBackground = new Color(getDisplay(), 255, 64, 64);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            errorBackground.dispose();
         }
      });
      
      final ModifyListener inputValidator = new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            try
            {
               Double.parseDouble(((Text)e.widget).getText().trim());
               ((Text)e.widget).setBackground(null);
            }
            catch(NumberFormatException ex)
            {
               ((Text)e.widget).setBackground(errorBackground);
            }
         }
      };
      from.getTextControl().addModifyListener(inputValidator);
      to.getTextControl().addModifyListener(inputValidator);
      
      onModeChange();
   }

   /**
    * Mode change handler
    */
   private void onModeChange()
   {
      boolean auto = radioAuto.getSelection();
      checkYBase.setEnabled(auto);
      from.setEnabled(!auto);
      to.setEnabled(!auto);
      if (auto)
      {
         from.getTextControl().setBackground(null);
         to.getTextControl().setBackground(null);
      }
      else
      {
         try
         {
            Double.parseDouble(from.getText().trim());
         }
         catch(NumberFormatException ex)
         {
            from.getTextControl().setBackground(errorBackground);
         }
         try
         {
            Double.parseDouble(to.getText().trim());
         }
         catch(NumberFormatException ex)
         {
            to.getTextControl().setBackground(errorBackground);
         }
      }
   }
   
   /**
    * Set selection
    * 
    * @param auto
    * @param minY
    * @param maxY
    */
   public void setSelection(boolean auto, boolean modifyYBase, double minY, double maxY)
   {
      radioAuto.setSelection(auto);
      checkYBase.setSelection((auto && modifyYBase));
      radioManual.setSelection(!auto);
      from.setText(Double.toString(minY));
      to.setText(Double.toString(maxY));
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
   public boolean modifyYBase()
   {
      return checkYBase.getSelection();
   }
   
   /**
    * @return
    */
   public double getMinY()
   {
      try
      {
         return Double.parseDouble(from.getText().trim());
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }
   
   /**
    * @return
    */
   public double getMaxY()
   {
      try
      {
         return Double.parseDouble(to.getText().trim());
      }
      catch(NumberFormatException e)
      {
         return 100;
      }
   }
   
   /**
    * Validate entered values
    * 
    * @param showErrorMessage if true error message will be shown to the user
    * @return true if values are valid
    */
   public boolean validate(boolean showErrorMessage)
   {
      if (isAuto())
         return true;
      
      try
      {
         Double.parseDouble(from.getText().trim());
         Double.parseDouble(to.getText().trim());
         return true;
      }
      catch(NumberFormatException e)
      {
         if (showErrorMessage)
            MessageDialogHelper.openWarning(getShell(), "Warning", "Y axis range is invalid. Please enter correct values.");
         return false;
      }
   }
}
