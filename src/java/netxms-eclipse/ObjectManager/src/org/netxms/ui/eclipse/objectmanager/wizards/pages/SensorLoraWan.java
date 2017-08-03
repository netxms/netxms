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
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.sensor.configs.LoraWanRegConfig;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.widgets.LoraWanWizard;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * LoRaWAN wizard page
 */
public class SensorLoraWan extends WizardPage 
{
   private Composite container;
   private LoraWanRegConfig conf;
   private LoraWanWizard loraWidget;

   public SensorLoraWan()
   {
      super(Messages.get().SensorWizard_LoRaWAN_Title);
      setTitle(Messages.get().SensorWizard_LoRaWAN_Title);
      setDescription(Messages.get().SensorWizard_LoRaWAN_Description);
   } 

   @Override
   public void createControl(Composite parent)
   {
      container = new Composite(parent, SWT.NONE); 
      conf = new LoraWanRegConfig();
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      container.setLayout(layout);        
     
      loraWidget = new LoraWanWizard(container, SWT.NONE, conf, getWizard());
      
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
      return false; // no next page exist
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.wizard.WizardPage#isPageComplete()
    */
   @Override
   public boolean isPageComplete()
   {
      if(!isCurrentPage())
         return true;
      return loraWidget.validate();
   }

   public String getRegConfig()
   {
      return loraWidget.getRegConfig();
   }

   public String getConfig()
   {
      return loraWidget.getConfig();
   }
}
