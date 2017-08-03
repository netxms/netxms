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
package org.netxms.ui.eclipse.objectmanager.widgets;

import org.eclipse.jface.wizard.IWizard;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.sensor.configs.LoraWanConfig;
import org.netxms.client.sensor.configs.LoraWanRegConfig;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
/**
 * LoRaWAN configuration
 */
public class LoraWanWizard extends Composite
{
   private Combo comboDecoder;
   private Combo comboRegistrationType;
   private LabeledText field2;
   private LabeledText field3;
   
   /**
    * Common sensor changeable field constructor
    *  
    * @param parent
    * @param style
    */
   public LoraWanWizard(Composite parent, int style, LoraWanRegConfig conf, final IWizard wizard)
   {
      super(parent, style); 
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);
      
      comboDecoder = WidgetHelper.createLabeledCombo(parent, SWT.BORDER | SWT.READ_ONLY, Messages.get().SensorWizard_LoRaWAN_Decoder, 
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      comboDecoder.setItems(LoraWanConfig.DECODER_NAMES);
      comboDecoder.select(0);
      
      comboRegistrationType = WidgetHelper.createLabeledCombo(parent, SWT.BORDER | SWT.READ_ONLY, Messages.get().SensorWizard_LoRaWAN_RegType, 
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      comboRegistrationType.setItems(LoraWanRegConfig.REG_OPTION_NAMES);
      comboRegistrationType.select(0);
      comboRegistrationType.addModifyListener(new ModifyListener() {
         
         @Override
         public void modifyText(ModifyEvent e)
         {
            if(comboRegistrationType.getSelectionIndex() == 0)
            {
               field2.setLabel(Messages.get().SensorWizard_LoRaWAN_AppEUI);
               field3.setLabel(Messages.get().SensorWizard_LoRaWAN_AppKey);
            }
            else
            {
               field2.setLabel(Messages.get().SensorWizard_LoRaWAN_NwkSkey);
               field3.setLabel(Messages.get().SensorWizard_LoRaWAN_AppSKey);
            }
            if(wizard != null)
               wizard.getContainer().updateButtons();
         }
      });
      comboRegistrationType.setEnabled(wizard != null);
      
      field2 = new LabeledText(parent, SWT.NONE);
      field2.setLabel(Messages.get().SensorWizard_LoRaWAN_AppEUI);
      field2.setText("");
      field2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      field2.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if(wizard != null)
               wizard.getContainer().updateButtons();
         }
      });
      field2.setEditable(wizard != null);
      
      field3 = new LabeledText(parent, SWT.NONE);
      field3.setLabel(Messages.get().SensorWizard_LoRaWAN_AppKey);
      field3.setText("");
      field3.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      field3.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if(wizard != null)
               wizard.getContainer().updateButtons();
         }
      });
      field3.setEditable(wizard != null);
   }
   
   public boolean validate()
   {
      if(comboRegistrationType.getSelectionIndex() == 0)
      {
         return field2.getText().length() == 16 && field3.getText().length() == 32;
      }
      return field2.getText().length() == 32 && field3.getText().length() == 32;         
   }
   
   public String getRegConfig()
   {
      LoraWanRegConfig regConf = new LoraWanRegConfig();
      regConf.registrationType = comboRegistrationType.getSelectionIndex();
      if(comboRegistrationType.getSelectionIndex() == 0)
      {
         regConf.appEUI = field2.getText();
         regConf.appKey = field3.getText();
      }
      else
      {
         regConf.nwkSKey = field2.getText();
         regConf.appSKey = field3.getText();
      }
      try
      {
         return regConf.createXml();
      }
      catch(Exception e)
      {
         e.printStackTrace();
         return null;
      }
   }
   
   public String getConfig()
   {
      LoraWanConfig conf = new LoraWanConfig();
      conf.decoder = comboDecoder.getSelectionIndex();
      try
      {
         return conf.createXml();
      }
      catch(Exception e)
      {
         e.printStackTrace();
         return null;
      }
   }
}
