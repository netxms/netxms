/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Locale;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.MacAddress;
import org.netxms.client.constants.SensorDeviceClass;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MacAddressValidator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ObjectNameValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Sensor object creation dialog
 */
public class CreateSensorDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSensorDialog.class);

	private LabeledText nameField;
   private LabeledText aliasField;
	private LabeledText macAddrField;
   private LabeledCombo deviceClassField;
   private ObjectSelector gatewayNodeField;
   private LabeledText deviceAddressField;
   private LabeledSpinner modbusUnitIdField;
   private LabeledText vendorField;
   private LabeledText modelField;
   private LabeledText serialNumberField;

	private String name;
   private String alias;
   private SensorDeviceClass deviceClass;
	private MacAddress macAddress;
   private String deviceAddress;
   private long gatewayNodeId;
   private short modbusUnitId;
   private String vendor;
   private String model;
   private String serialNumber;

	/**
	 * @param parentShell
	 */
	public CreateSensorDialog(Shell parentShell)
	{
		super(parentShell);
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Sensor"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
		gd.horizontalSpan = 2;
		nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      aliasField.setLayoutData(gd);

      deviceClassField = new LabeledCombo(dialogArea, SWT.NONE);
      deviceClassField.setLabel(i18n.tr("Device class"));
      deviceClassField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      Locale locale = LocalizationHelper.getLocale();
      for(SensorDeviceClass c : SensorDeviceClass.class.getEnumConstants())
         deviceClassField.add(c.getDisplayName(locale));
      deviceClassField.select(0);

      gatewayNodeField = new ObjectSelector(dialogArea, SWT.NONE, true);
      gatewayNodeField.setLabel(i18n.tr("Gateway node"));
      gatewayNodeField.setObjectClass(Node.class);
      gatewayNodeField.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
      gatewayNodeField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

		macAddrField = new LabeledText(dialogArea, SWT.NONE);
      macAddrField.setLabel(i18n.tr("MAC address"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		macAddrField.setLayoutData(gd);

      modbusUnitIdField = new LabeledSpinner(dialogArea, SWT.NONE);
      modbusUnitIdField.setLabel(i18n.tr("Modbus unit ID"));
      modbusUnitIdField.setRange(0, 255);
      modbusUnitIdField.setSelection(255);

      deviceAddressField = new LabeledText(dialogArea, SWT.NONE);
      deviceAddressField.setLabel(i18n.tr("Device address"));
      deviceAddressField.getTextControl().setTextLimit(255);
      deviceAddressField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      vendorField = new LabeledText(dialogArea, SWT.NONE);
      vendorField.setLabel(i18n.tr("Vendor"));
      vendorField.getTextControl().setTextLimit(255);
      vendorField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      modelField = new LabeledText(dialogArea, SWT.NONE);
      modelField.setLabel(i18n.tr("Model"));
      modelField.getTextControl().setTextLimit(255);
      modelField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      serialNumberField = new LabeledText(dialogArea, SWT.NONE);
      serialNumberField.setLabel(i18n.tr("Serial number"));
      serialNumberField.getTextControl().setTextLimit(63);
      serialNumberField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		if (!WidgetHelper.validateTextInput(nameField, new ObjectNameValidator()) ||
            !WidgetHelper.validateTextInput(macAddrField, new MacAddressValidator(true)))
      {
         WidgetHelper.adjustWindowSize(this);
			return;
      }

		try
		{
			name = nameField.getText().trim();
         alias = aliasField.getText().trim();
         deviceClass = SensorDeviceClass.getByValue(deviceClassField.getSelectionIndex());
         gatewayNodeId = gatewayNodeField.getObjectId();
			macAddress = macAddrField.getText().trim().isEmpty() ? new MacAddress() : MacAddress.parseMacAddress(macAddrField.getText());
         modbusUnitId = (short)modbusUnitIdField.getSelection();
         deviceAddress = deviceAddressField.getText().trim();
         vendor = vendorField.getText().trim();
         model = modelField.getText().trim();
         serialNumber = serialNumberField.getText().trim();

			super.okPressed();
		}
		catch(Exception e)
		{
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Internal error: %s"), e.getLocalizedMessage()));
         WidgetHelper.adjustWindowSize(this);
		}
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

   /**
    * @return the deviceClass
    */
   public SensorDeviceClass getDeviceClass()
   {
      return deviceClass;
   }

   /**
    * @return the deviceAddress
    */
   public String getDeviceAddress()
   {
      return deviceAddress;
   }

   /**
    * @return the gatewayNodeId
    */
   public long getGatewayNodeId()
   {
      return gatewayNodeId;
   }

   /**
    * @return the modbusUnitId
    */
   public short getModbusUnitId()
   {
      return modbusUnitId;
   }

   /**
    * @return the vendor
    */
   public String getVendor()
   {
      return vendor;
   }

   /**
    * @return the model
    */
   public String getModel()
   {
      return model;
   }

   /**
    * @return the serialNumber
    */
   public String getSerialNumber()
   {
      return serialNumber;
   }
}
