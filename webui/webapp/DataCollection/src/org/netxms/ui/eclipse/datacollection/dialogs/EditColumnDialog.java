/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Column definition editing dialog
 */
public class EditColumnDialog extends Dialog
{
	private ColumnDefinition column;
	private LabeledText name;
	private LabeledText displayName;
	private Combo dataType;
	private Combo aggregationFunction;
	private Button checkInstanceColumn;
	private Button checkInstanceLabelColumn;
	private LabeledText snmpOid;
	
	/**
	 * @param parentShell
	 */
	public EditColumnDialog(Shell parentShell, ColumnDefinition column)
	{
		super(parentShell);
		this.column = column;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().EditColumnDialog_ColumnDefinition + column.getName());
	}

	/* (non-Javadoc)
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
		name.setLabel("Name");
		name.setText(column.getName());
		name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		
		displayName = new LabeledText(dialogArea, SWT.NONE);
		displayName.setLabel("Display name");
		displayName.setText(column.getDisplayName());
		displayName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		
		dataType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Data type", new GridData(SWT.FILL, SWT.CENTER, true, false));
		dataType.add(Messages.get().TableColumnLabelProvider_in32);
		dataType.add(Messages.get().TableColumnLabelProvider_uint32);
		dataType.add(Messages.get().TableColumnLabelProvider_int64);
		dataType.add(Messages.get().TableColumnLabelProvider_uint64);
		dataType.add(Messages.get().TableColumnLabelProvider_string);
		dataType.add(Messages.get().TableColumnLabelProvider_float);
		dataType.select(column.getDataType());
		
		aggregationFunction = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Aggregation function", new GridData(SWT.FILL, SWT.CENTER, true, false));
		aggregationFunction.add(Messages.get().TableColumnLabelProvider_SUM);
		aggregationFunction.add(Messages.get().TableColumnLabelProvider_AVG);
		aggregationFunction.add(Messages.get().TableColumnLabelProvider_MIN);
		aggregationFunction.add(Messages.get().TableColumnLabelProvider_MAX);
		aggregationFunction.select(column.getAggregationFunction());
		
		checkInstanceColumn = new Button(dialogArea, SWT.CHECK);
		checkInstanceColumn.setText("This column is instance (key) column");
		checkInstanceColumn.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		checkInstanceColumn.setSelection(column.isInstanceColumn());
		
		checkInstanceLabelColumn = new Button(dialogArea, SWT.CHECK);
		checkInstanceLabelColumn.setText("This column is instance label column");
		checkInstanceLabelColumn.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		checkInstanceLabelColumn.setSelection(column.isInstanceLabelColumn());
		
		snmpOid = new LabeledText(dialogArea, SWT.NONE);
		snmpOid.setLabel("SNMP Object ID");
		snmpOid.setText((column.getSnmpObjectId() != null) ? column.getSnmpObjectId().toString() : ""); 
		snmpOid.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
		
		return dialogArea;
	}

	/* (non-Javadoc)
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
				MessageDialogHelper.openWarning(getShell(), "Warning", "Entered SNMP object ID is invalid");
				return;
			}
		}
		else
		{
			column.setSnmpObjectId(null);
		}
		
		column.setName(name.getText().trim());
		column.setDisplayName(displayName.getText().trim());
		column.setInstanceColumn(checkInstanceColumn.getSelection());
		column.setInstanceLabelColumn(checkInstanceLabelColumn.getSelection());
		column.setDataType(dataType.getSelectionIndex());
		column.setAggregationFunction(aggregationFunction.getSelectionIndex());
		
		super.okPressed();
	}
}
