/**
 * NetXMS - open source network management system
 * Copyright (C) 2017 Raden Solutions
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
package org.netxms.ui.eclipse.objectmanager.wizards.pages;

import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Sensor;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.widgets.SensorCommon;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * General sensor configuration wizard page
 */
public class SensorGeneralPage extends WizardPage 
{
   private Composite container;
   private LabeledText textObjectName;
   private Combo comboCommMethod;
   private SensorCommon commonData;
   
   /**
    * Constructor for Sensor general property page
    */
   public SensorGeneralPage()
   {
      super(Messages.get().SensorWizard_General_Title);
      setTitle(Messages.get().SensorWizard_General_Title);
      setDescription(Messages.get().SensorWizard_General_Description);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.IDialogPage#createControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createControl(Composite parent)
   {
      container = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      container.setLayout(layout);
      
      textObjectName = new LabeledText(container, SWT.NONE);
      textObjectName.setLabel(Messages.get().General_ObjectName);
      textObjectName.setText("");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textObjectName.setLayoutData(gd);
      textObjectName.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            getWizard().getContainer().updateButtons();
         }
      });
      
      comboCommMethod = (Combo)WidgetHelper.createLabeledCombo(container, SWT.BORDER | SWT.READ_ONLY, Messages.get().SensorWizard_General_CommMethod, 
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      comboCommMethod.setItems(Sensor.COMM_METHOD);
      comboCommMethod.select(0);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = false;
      comboCommMethod.setLayoutData(gd);
      comboCommMethod.addModifyListener(new ModifyListener() {         
         @Override
         public void modifyText(ModifyEvent e)
         {
            getWizard().getContainer().updateButtons();
         }
      });
      
      commonData = new SensorCommon(container, SWT.NONE, getWizard());
      
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      commonData.setLayoutData(gd);

      setControl(container); 
      
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.wizard.WizardPage#canFlipToNextPage()
    */
   @Override
   public boolean canFlipToNextPage()
   {
      if(!isCurrentPage())
         return true;
      if(textObjectName.getText().isEmpty())
         return false;
      
      return commonData.validate();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.wizard.WizardPage#isPageComplete()
    */
   @Override
   public boolean isPageComplete()
   {
      if(!isCurrentPage())
         return true;
      if(textObjectName.getText().isEmpty())
         return false;
      if(comboCommMethod.getSelectionIndex() != Sensor.COMM_LORAWAN)
         return true;
      return false;
   }

   /**
    * @return the comboCommMethod
    */
   public int getCommMethod()
   {
      return comboCommMethod.getSelectionIndex();
   }

   /**
    * @return the textObjectName
    */
   public String getObjectName()
   {
      return textObjectName.getText();
   }

   /**
    * @return the commonData
    */
   public SensorCommon getCommonData()
   {
      return commonData;
   }
}
