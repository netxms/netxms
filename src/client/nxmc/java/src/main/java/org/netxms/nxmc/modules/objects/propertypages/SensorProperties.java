/**
 * NetXMS - open source network management system
 * Copyright (C) 2017-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.propertypages;

import java.util.Locale;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.SensorDeviceClass;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

public class SensorProperties extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(SensorProperties.class);

   private Sensor sensor;
   private LabeledCombo deviceClass;
   private ObjectSelector gatewayNode;
   private LabeledText macAddress;
   private LabeledText deviceAddress;
   private LabeledSpinner modbusUnitId;
   private LabeledText vendor;
   private LabeledText model;
   private LabeledText serialNumber;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public SensorProperties(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(SensorProperties.class).tr("Sensor"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "sensorProperties";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Sensor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);      
      sensor = (Sensor)object;
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      deviceClass = new LabeledCombo(dialogArea, SWT.NONE);
      deviceClass.setLabel(i18n.tr("Device class"));
      deviceClass.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      Locale locale = LocalizationHelper.getLocale();
      for(SensorDeviceClass c : SensorDeviceClass.class.getEnumConstants())
         deviceClass.add(c.getDisplayName(locale));
      deviceClass.select(sensor.getDeviceClass().getValue());

      gatewayNode = new ObjectSelector(dialogArea, SWT.NONE, true);
      gatewayNode.setLabel(i18n.tr("Gateway node"));
      gatewayNode.setObjectClass(Node.class);
      gatewayNode.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
      gatewayNode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      gatewayNode.setObjectId(sensor.getGatewayId());

      macAddress = new LabeledText(dialogArea, SWT.NONE);
      macAddress.setLabel(i18n.tr("MAC address"));
      macAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      macAddress.setText(sensor.getMacAddress().toString());

      modbusUnitId = new LabeledSpinner(dialogArea, SWT.NONE);
      modbusUnitId.setLabel(i18n.tr("Modbus unit ID"));
      modbusUnitId.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      modbusUnitId.setRange(0, 255);
      modbusUnitId.setSelection(sensor.getModbusUnitId());

      deviceAddress = new LabeledText(dialogArea, SWT.NONE);
      deviceAddress.setLabel(i18n.tr("Device address"));
      deviceAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      deviceAddress.setText(sensor.getDeviceAddress());

      vendor = new LabeledText(dialogArea, SWT.NONE);
      vendor.setLabel(i18n.tr("Vendor"));
      vendor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      vendor.setText(sensor.getVendor());

      model = new LabeledText(dialogArea, SWT.NONE);
      model.setLabel(i18n.tr("Model"));
      model.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      model.setText(sensor.getModel());

      serialNumber = new LabeledText(dialogArea, SWT.NONE);
      serialNumber.setLabel(i18n.tr("Serial number"));
      serialNumber.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      serialNumber.setText(sensor.getSerialNumber());

      return dialogArea;
   }
   
   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(sensor.getObjectId());

      String macAddressText = macAddress.getText().trim();
      if (!macAddressText.isEmpty())
      {
         try
         {
            md.setMacAddress(MacAddress.parseMacAddress(macAddress.getText().trim()));
         }
         catch(MacAddressFormatException e)
         {
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Invalid MAC address"));
            return false;
         }
      }
      else
      {
         md.setMacAddress(new MacAddress());
      }
      md.setDeviceClass(SensorDeviceClass.getByValue(deviceClass.getSelectionIndex()));
      md.setVendor(vendor.getText().trim());
      md.setModel(model.getText().trim());
      md.setSerialNumber(serialNumber.getText().trim());
      md.setDeviceAddress(deviceAddress.getText().trim());
      md.setModbusUnitId((short)modbusUnitId.getSelection());
      md.setSensorProxy(gatewayNode.getObjectId());

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating sensor configuration"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update sensor configuration");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> SensorProperties.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      gatewayNode.setObjectId(0);
      deviceClass.select(0);
      macAddress.setText("");
      deviceAddress.setText("");
      modbusUnitId.setSelection(255);
      vendor.setText("");
      model.setText("");
      serialNumber.setText("");
   }
}
