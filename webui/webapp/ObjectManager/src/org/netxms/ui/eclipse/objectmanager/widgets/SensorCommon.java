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
import org.netxms.client.MacAddress;
import org.netxms.client.MacAddressFormatException;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Sensor;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
/**
 * Common sensor changeable fields
 */
public class SensorCommon extends Composite
{
   private LabeledText textMacAddress;
   private Combo comboDeviceClass;
   private LabeledText textVendor;
   private LabeledText textSerial;
   private LabeledText textDeviceAddress;
   private LabeledText textMetaType;
   private LabeledText textDescription;
   private ObjectSelector selectorProxyNode;
   private ModifyListener modifyListener = null;
   
   public SensorCommon(Composite parent, int style, IWizard wizard)
   {
      this(parent, style, wizard, "", 0, "", "", "", "", "",0,0);
   }
   
   /**
    * Common sensor changeable field constructor
    *  
    * @param parent
    * @param style
    */
   public SensorCommon(Composite parent, int style, final IWizard wizard, String mac, int devClass, String vendor, String serial, String devAddress, String metaType, String desc, long proxyNodeId, int commProto)
   {
      super(parent, style);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      setLayout(layout);
      
      selectorProxyNode = new ObjectSelector(this, SWT.NONE, true);
      selectorProxyNode.setLabel(Messages.get().SensorWizard_General_Proxy);
      selectorProxyNode.setObjectClass(Node.class);
      selectorProxyNode.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
      selectorProxyNode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      if(proxyNodeId != 0)
         selectorProxyNode.setObjectId(proxyNodeId);
      
      textMacAddress = new LabeledText(this, SWT.NONE);
      textMacAddress.setLabel(Messages.get().SensorWizard_General_MacAddr);
      textMacAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textMacAddress.setEditable(commProto != Sensor.COMM_LORAWAN);
      
      comboDeviceClass = WidgetHelper.createLabeledCombo(this, SWT.BORDER | SWT.READ_ONLY, Messages.get().SensorWizard_General_DeviceClass, 
            WidgetHelper.DEFAULT_LAYOUT_DATA);
      comboDeviceClass.setItems(Sensor.DEV_CLASS_NAMES);
      comboDeviceClass.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      textVendor = new LabeledText(this, SWT.NONE);
      textVendor.setLabel(Messages.get().SensorWizard_General_Vendor);
      textVendor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      textSerial = new LabeledText(this, SWT.NONE);
      textSerial.setLabel(Messages.get().SensorWizard_General_Serial);
      textSerial.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      textDeviceAddress = new LabeledText(this, SWT.NONE);
      textDeviceAddress.setLabel(Messages.get().SensorWizard_General_DeviceAddr);
      textDeviceAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textDeviceAddress.setEnabled(commProto != Sensor.COMM_LORAWAN);
      
      textMetaType = new LabeledText(this, SWT.NONE);
      textMetaType.setLabel(Messages.get().SensorWizard_General_MetaType);
      textMetaType.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      textDescription = new LabeledText(this, SWT.NONE);
      textDescription.setLabel(Messages.get().SensorWizard_General_DescrLabel);
      textDescription.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      updateFields(mac, devClass, vendor, serial, devAddress, metaType, desc);
      
      if (wizard != null)
      {
         modifyListener = new ModifyListener() {         
            @Override
            public void modifyText(ModifyEvent e)
            {
               if (wizard != null)
                  wizard.getContainer().updateButtons();
            }
         };
         selectorProxyNode.addModifyListener(modifyListener);
         textMacAddress.getTextControl().addModifyListener(modifyListener);
         textDeviceAddress.getTextControl().addModifyListener(modifyListener);
      }
      
      
   }
   
   public void updateFields(String mac, int devClass, String vendor, String serial, String devAddress, String metaType, String desc)
   {
      textMacAddress.setText(mac);
      comboDeviceClass.select(devClass);
      textVendor.setText(vendor);
      textSerial.setText(serial);
      textDeviceAddress.setText(devAddress);
      textMetaType.setText(metaType);
      textDescription.setText(desc);
      
   }
   
   /**
    * Check if all required fields are filled
    * @return true if page is valid
    */
   public boolean validate()
   {
      return (selectorProxyNode.getObjectId() != 0)
            && ((textMacAddress.getText().length() >= 12
            && textMacAddress.getText().length() <= 23)
            || (textDeviceAddress.getText().length() > 0));
   }

   /**
    * @return the textMacAddress value as MacAddress object
    */
   public MacAddress getMacAddress()
   {
      if(textMacAddress.getText().isEmpty())
         return null;
      try
      {
         return MacAddress.parseMacAddress(textMacAddress.getText());
      }
      catch(MacAddressFormatException e)
      {
         return null;
      }
   }

   /**
    * @return the comboDeviceClass selection index
    */
   public int getDeviceClass()
   {
      return comboDeviceClass.getSelectionIndex();
   }

   /**
    * @return the textVendor input text
    */
   public String getVendor()
   {
      return textVendor.getText();
   }

   /**
    * @return the textSerial input text
    */
   public String getSerial()
   {
      return textSerial.getText();
   }

   /**
    * @return the textDeviceAddress input text
    */
   public String getDeviceAddress()
   {
      return textDeviceAddress.getText();
   }

   /**
    * @return the textMetaType input text
    */
   public String getMetaType()
   {
      return textMetaType.getText();
   }

   /**
    * @return the textDescription input text
    */
   public String getDescription()
   {
      return textDescription.getText();
   }
   
   public long getProxyNode()
   {
      return selectorProxyNode.getObjectId();
   }
}
