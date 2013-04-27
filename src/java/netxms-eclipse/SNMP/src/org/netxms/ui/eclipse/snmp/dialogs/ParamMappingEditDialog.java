/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpTrapParameterMapping;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Trap parameter mapping edit dialog
 *
 */
public class ParamMappingEditDialog extends Dialog
{
	private SnmpTrapParameterMapping pm;
	private Text description;
	private Button radioByOid;
	private Button radioByPosition;
	private Text objectId;
	private Button buttonSelect;
	private Spinner position;
	private Button checkForceText;

	/**
	 * Create dialog.
	 * 
	 * @param parentShell parent shell
	 * @param pm trap parameter mapping to be edited
	 */
	public ParamMappingEditDialog(Shell parentShell, SnmpTrapParameterMapping pm)
	{
		super(parentShell);
		this.pm = pm;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.ParamMappingEditDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		description = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.ParamMappingEditDialog_Description, pm.getDescription(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		Group varbind = new Group(dialogArea, SWT.NONE);
		varbind.setText(Messages.ParamMappingEditDialog_Varbind);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		varbind.setLayoutData(gd);
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		varbind.setLayout(layout);
		
		radioByOid = new Button(varbind, SWT.RADIO);
		radioByOid.setText(Messages.ParamMappingEditDialog_ByOID);
		radioByOid.setSelection(pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID);
		radioByOid.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				enableControls(radioByOid.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		Composite oidSelection = new Composite(varbind, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		oidSelection.setLayout(layout);
		
		objectId = new Text(oidSelection, SWT.BORDER);
		if (pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID)
			objectId.setText(pm.getObjectId().toString());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		objectId.setLayoutData(gd);
		
		buttonSelect = new Button(oidSelection, SWT.PUSH);
		buttonSelect.setText(Messages.ParamMappingEditDialog_Select);
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		buttonSelect.setLayoutData(gd);
		buttonSelect.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectObjectId();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		radioByPosition = new Button(varbind, SWT.RADIO);
		radioByPosition.setText(Messages.ParamMappingEditDialog_ByPos);
		radioByPosition.setSelection(pm.getType() == SnmpTrapParameterMapping.BY_POSITION);
		radioByPosition.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				enableControls(!radioByPosition.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		Composite positionSelection = new Composite(varbind, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		positionSelection.setLayout(layout);
		
		position = new Spinner(positionSelection, SWT.BORDER);
		position.setIncrement(1);
		position.setMaximum(255);
		position.setMinimum(1);
		gd = new GridData();
		gd.widthHint = 40;
		position.setLayoutData(gd);
		
		new Label(positionSelection, SWT.NONE).setText(Messages.ParamMappingEditDialog_EnterVarbindPos);

		final Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.ParamMappingEditDialog_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		
		checkForceText = new Button(optionsGroup, SWT.CHECK);
		checkForceText.setText(Messages.ParamMappingEditDialog_NeverConvertToHex);
		checkForceText.setSelection((pm.getFlags() & SnmpTrapParameterMapping.FORCE_TEXT) != 0);
		
		enableControls(pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID);
		
		return dialogArea;
	}
	
	/**
	 * Select OID using MIB selection dialog
	 */
	private void selectObjectId()
	{
		SnmpObjectId oid;
		try
		{
			oid = SnmpObjectId.parseSnmpObjectId(objectId.getText());
		}
		catch(SnmpObjectIdFormatException e)
		{
			oid = null;
		}
		MibSelectionDialog dlg = new MibSelectionDialog(getShell(), oid, 0);
		if (dlg.open() == Window.OK)
		{
			objectId.setText(dlg.getSelectedObjectId().toString());
			objectId.setFocus();
		}
	}
	
	/**
	 * Enable or disable dialog controls depending on mapping type
	 * 
	 * @param mappingByOid true if current mapping type is "by OID"
	 */
	private void enableControls(boolean mappingByOid)
	{
		objectId.setEnabled(mappingByOid);
		buttonSelect.setEnabled(mappingByOid);
		position.setEnabled(!mappingByOid);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		int type = radioByOid.getSelection() ? SnmpTrapParameterMapping.BY_OBJECT_ID : SnmpTrapParameterMapping.BY_POSITION;
		if (type == SnmpTrapParameterMapping.BY_OBJECT_ID)
		{
			try
			{
				pm.setObjectId(SnmpObjectId.parseSnmpObjectId(objectId.getText()));
			}
			catch(SnmpObjectIdFormatException e)
			{
				MessageDialogHelper.openWarning(getShell(), Messages.ParamMappingEditDialog_Warning, Messages.ParamMappingEditDialog_WarningInvalidOID);
				return;
			}
		}
		else
		{
			pm.setPosition(position.getSelection());
		}
		pm.setType(type);
		pm.setDescription(description.getText());
		pm.setFlags(checkForceText.getSelection() ? SnmpTrapParameterMapping.FORCE_TEXT : 0);
		super.okPressed();
	}
}
