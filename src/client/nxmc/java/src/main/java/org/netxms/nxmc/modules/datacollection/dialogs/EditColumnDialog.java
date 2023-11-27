/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Column definition editing dialog
 */
public class EditColumnDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditColumnDialog.class);
   
	private ColumnDefinition column;
	private LabeledText name;
	private LabeledText displayName;
	private Combo dataType;
	private Combo aggregationFunction;
	private Button checkInstanceColumn;
   private Button checkSnmpHexString;
	private LabeledText snmpOid;

	/**
	 * @param parentShell
	 */
   public EditColumnDialog(Shell parentShell, ColumnDefinition column)
	{
		super(parentShell);
		this.column = column;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(column.getName().isEmpty() ? i18n.tr("Add Column Definition") : i18n.tr("Edit Column Definition "));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel(i18n.tr("Name"));
		name.setText(column.getName());
		name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

		displayName = new LabeledText(dialogArea, SWT.NONE);
		displayName.setLabel(i18n.tr("Display name"));
		displayName.setText(column.getDisplayName());
		displayName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		
		dataType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Data type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.FLOAT));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.STRING));
		dataType.select(getDataTypePosition(column.getDataType()));
		
		aggregationFunction = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Aggregation function"), new GridData(SWT.FILL, SWT.CENTER, true, false));
		aggregationFunction.add(i18n.tr("SUM"));
		aggregationFunction.add(i18n.tr("AVG"));
		aggregationFunction.add(i18n.tr("MIN"));
		aggregationFunction.add(i18n.tr("MAX"));
		aggregationFunction.select(column.getAggregationFunction());
		
		checkInstanceColumn = new Button(dialogArea, SWT.CHECK);
		checkInstanceColumn.setText(i18n.tr("This column is instance (key) column"));
		checkInstanceColumn.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		checkInstanceColumn.setSelection(column.isInstanceColumn());
		
      checkSnmpHexString = new Button(dialogArea, SWT.CHECK);
      checkSnmpHexString.setText("Convert SNMP value to &hexadecimal string");
      checkSnmpHexString.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      checkSnmpHexString.setSelection(column.isConvertSnmpStringToHex());
      
		snmpOid = new LabeledText(dialogArea, SWT.NONE);
		snmpOid.setLabel(i18n.tr("SNMP Object ID"));
		snmpOid.setText((column.getSnmpObjectId() != null) ? column.getSnmpObjectId().toString() : "");  //$NON-NLS-1$
		snmpOid.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		String oidText = snmpOid.getText().trim();
		if (!oidText.isEmpty())
		{
			try
			{
				SnmpObjectId oid = SnmpObjectId.parseSnmpObjectId(oidText);
				column.setSnmpObjectId(oid);
			}
			catch(SnmpObjectIdFormatException e)
			{
				MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Entered SNMP object ID is invalid"));
				return;
			}
		}
		else
		{
			column.setSnmpObjectId(null);
		}

      if (name.getText().trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Column name can not be empty"));
         return;
      }

      column.setName(name.getText().trim());
      column.setDisplayName(displayName.getText().trim().isEmpty() ? name.getText().trim() : displayName.getText().trim());
      column.setInstanceColumn(checkInstanceColumn.getSelection());
		column.setConvertSnmpStringToHex(checkSnmpHexString.getSelection());
		column.setDataType(getDataTypeByPosition(dataType.getSelectionIndex()));
		column.setAggregationFunction(aggregationFunction.getSelectionIndex());

		super.okPressed();
	}

   /**
    * Get selector position for given data type
    * 
    * @param type data type
    * @return corresponding selector's position
    */
   private static int getDataTypePosition(DataType type)
   {
      switch(type)
      {
         case COUNTER32:
            return 2;
         case COUNTER64:
            return 5;
         case FLOAT:
            return 6;
         case INT32:
            return 0;
         case INT64:
            return 3;
         case STRING:
            return 7;
         case UINT32:
            return 1;
         case UINT64:
            return 4;
         default:
            return 0;  // fallback to int32
      }
   }

   /**
    * Data type positions in selector
    */
   private static final DataType[] TYPES = {
      DataType.INT32, DataType.UINT32, DataType.COUNTER32, DataType.INT64,
      DataType.UINT64, DataType.COUNTER64, DataType.FLOAT, DataType.STRING
   };

   /**
    * Get data type by selector position
    *  
    * @param position selector position
    * @return corresponding data type
    */
   private static DataType getDataTypeByPosition(int position)
   {
      return TYPES[position];
   }
}
