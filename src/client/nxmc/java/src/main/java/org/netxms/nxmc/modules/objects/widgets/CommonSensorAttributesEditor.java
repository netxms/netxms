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
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.wizard.IWizard;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Common sensor changeable fields
 */
public class CommonSensorAttributesEditor extends Composite
{
   private I18n i18n = LocalizationHelper.getI18n(CommonSensorAttributesEditor.class);
   private LabeledText textMacAddress;
   private Combo comboDeviceClass;
   private LabeledText textVendor;
   private LabeledText textSerial;
   private LabeledText textDeviceAddress;
   private LabeledText textMetaType;
   private LabeledText textDescription;
   private ObjectSelector selectorProxyNode;
   private ModifyListener modifyListener = null;
   private int commProtocol;

   public CommonSensorAttributesEditor(Composite parent, int style, IWizard wizard)
   {
      this(parent, style, wizard, "", 0, "", "", "", "", "", 0, 0);
   }

   /**
    * Common sensor changeable field constructor
    * 
    * @param parent
    * @param style
    */
   public CommonSensorAttributesEditor(Composite parent, int style, final IWizard wizard, String mac, int devClass, String vendor, String serial,
         String devAddress, String metaType, String desc, long proxyNodeId, int commProto)
   {
      super(parent, style);
      commProtocol = commProto;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      setLayout(layout);

      selectorProxyNode = new ObjectSelector(this, SWT.NONE, true);
      selectorProxyNode.setLabel(i18n.tr("Proxy node"));
      selectorProxyNode.setObjectClass(Node.class);
      selectorProxyNode.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
      selectorProxyNode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      if (proxyNodeId != 0)
         selectorProxyNode.setObjectId(proxyNodeId);

      textMacAddress = new LabeledText(this, SWT.NONE);
      textMacAddress.setLabel(i18n.tr("MAC address"));
      textMacAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textMacAddress.setEditable(commProto != Sensor.COMM_LORAWAN);

      comboDeviceClass = WidgetHelper.createLabeledCombo(this, SWT.BORDER | SWT.READ_ONLY,
            i18n.tr("Device class"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      comboDeviceClass.setItems(Sensor.DEV_CLASS_NAMES);
      comboDeviceClass.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textVendor = new LabeledText(this, SWT.NONE);
      textVendor.setLabel(i18n.tr("Vendor"));
      textVendor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textSerial = new LabeledText(this, SWT.NONE);
      textSerial.setLabel(i18n.tr("Serial number"));
      textSerial.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textDeviceAddress = new LabeledText(this, SWT.NONE);
      textDeviceAddress.setLabel(i18n.tr("Device address"));
      textDeviceAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textDeviceAddress.setEnabled(commProto != Sensor.COMM_LORAWAN);

      textMetaType = new LabeledText(this, SWT.NONE);
      textMetaType.setLabel(i18n.tr("Meta type"));
      textMetaType.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textDescription = new LabeledText(this, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
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
    * 
    * @return true if page is valid
    */
   public boolean validate()
   {   
      if(commProtocol == Sensor.COMM_LORAWAN && 
            (textMacAddress.getText() == null || textDeviceAddress.getText().isEmpty()))
         return false;
      if(commProtocol != Sensor.SENSOR_PROTO_UNKNOWN && selectorProxyNode.getObjectId() == 0)
         return false;
      if(textMacAddress.getText().length() > 0 && !(textMacAddress.getText().length() >= 12
            && textMacAddress.getText().length() <= 23))
         return false;
      return true;
   }

   /**
    * @return the textMacAddress value as MacAddress object
    */
   public MacAddress getMacAddress()
   {
      if (textMacAddress.getText().isEmpty())
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
