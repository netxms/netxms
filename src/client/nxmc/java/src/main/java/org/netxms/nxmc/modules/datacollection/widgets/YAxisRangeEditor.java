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
package org.netxms.nxmc.modules.datacollection.widgets;

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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Control group to edit Y axis range settings in chart
 */
public class YAxisRangeEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(YAxisRangeEditor.class);

   private Button radioAuto;
   private Button radioManual;
   private Button checkYBase;
   private LabeledText from;
   private LabeledText to;
   private LabeledText label;
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
      group.setText(i18n.tr("Y Axis Settings"));

      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      group.setLayout(layout);

      radioAuto = new Button(group, SWT.RADIO);
      radioAuto.setText(i18n.tr("Automatic range"));
      radioAuto.setSelection(true);

      Label spacer = new Label(group, SWT.NONE);
      GridData gd = new GridData();
      gd.widthHint = 20;
      spacer.setLayoutData(gd);

      checkYBase = new Button(group, SWT.CHECK);
      checkYBase.setText(i18n.tr("Set Y base to min value"));

      radioManual = new Button(group, SWT.RADIO);
      radioManual.setText(i18n.tr("Manual range"));

      from = new LabeledText(group, SWT.NONE);
      from.setLabel(i18n.tr("From"));
      from.setText("0");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      from.setLayoutData(gd);

      to = new LabeledText(group, SWT.NONE);
      to.setLabel(i18n.tr("To"));
      to.setText("100");
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      to.setLayoutData(gd);

      label = new LabeledText(group, SWT.NONE);
      label.setLabel(i18n.tr("Axis label (leave empty to hide)"));
      label.setText("");
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      label.setLayoutData(gd);

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
    * @param modifyYBase
    * @param minY
    * @param maxY
    * @param labelText
    */
   public void setSelection(boolean auto, boolean modifyYBase, double minY, double maxY, String labelText)
   {
      radioAuto.setSelection(auto);
      checkYBase.setSelection((auto && modifyYBase));
      radioManual.setSelection(!auto);
      from.setText(Double.toString(minY));
      to.setText(Double.toString(maxY));
      label.setText(labelText);
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
    * Get label text.
    *
    * @return label text
    */
   public String getLabel()
   {
      return label.getText().trim();
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
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
                  i18n.tr("Y axis range is invalid. Please enter correct values."));
         return false;
      }
   }
}
