/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.client.snmp.SnmpTrapParameterMapping;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.helpers.ParamMappingLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Trap configuration dialog
 *
 */
public class TrapConfigurationDialog extends Dialog
{
	private static final String PARAMLIST_TABLE_SETTINGS = "TrapConfigurationDialog.ParamList"; //$NON-NLS-1$
	
	private SnmpTrap trap;
	private List<SnmpTrapParameterMapping> pmap;
	private Text description;
	private Text oid;
	private EventSelector event;
	private Text eventTag;
	private TableViewer paramList;
	private Button buttonAdd;
	private Button buttonEdit;
	private Button buttonDelete;
	private Button buttonUp;
	private Button buttonDown;
	private Button buttonSelect;

	/**
	 * Creates trap configuration dialog
	 * 
	 * @param parentShell parent shell
	 * @param trap SNMP trap object to edit
	 */
	public TrapConfigurationDialog(Shell parentShell, SnmpTrap trap)
	{
		super(parentShell);
		this.trap = trap;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.TrapConfigurationDialog_Title);
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
		
		description = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.TrapConfigurationDialog_Description, trap.getDescription(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		Composite oidSelection = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		oidSelection.setLayout(layout);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		oidSelection.setLayoutData(gd);
		
		oid = WidgetHelper.createLabeledText(oidSelection, SWT.BORDER, 300, Messages.TrapConfigurationDialog_TrapOID, 
				(trap.getObjectId() != null) ? trap.getObjectId().toString() : "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$

		buttonSelect = new Button(oidSelection, SWT.PUSH);
		buttonSelect.setText(Messages.TrapConfigurationDialog_Select);
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		gd.verticalAlignment = SWT.BOTTOM;
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
		
		event = new EventSelector(dialogArea, SWT.NONE);
		event.setLabel(Messages.TrapConfigurationDialog_Event);
		event.setEventCode(trap.getEventCode());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		event.setLayoutData(gd);
		
		eventTag = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, SWT.DEFAULT, Messages.TrapConfigurationDialog_UserTag, trap.getUserTag(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText(Messages.TrapConfigurationDialog_Parameters);
		
		Composite paramArea = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		paramArea.setLayoutData(gd);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		paramArea.setLayout(layout);

		paramList = new TableViewer(paramArea, SWT.BORDER | SWT.FULL_SELECTION);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 300;
		paramList.getTable().setLayoutData(gd);
		setupParameterList();
		
		Composite buttonArea = new Composite(paramArea, SWT.NONE);
		RowLayout btnLayout = new RowLayout();
		btnLayout.type = SWT.VERTICAL;
		btnLayout.marginBottom = 0;
		btnLayout.marginLeft = 0;
		btnLayout.marginRight = 0;
		btnLayout.marginTop = 0;
		btnLayout.fill = true;
		btnLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttonArea.setLayout(btnLayout);
		
		buttonAdd = new Button(buttonArea, SWT.PUSH);
		buttonAdd.setText(Messages.TrapConfigurationDialog_Add);
		buttonAdd.setLayoutData(new RowData(WidgetHelper.BUTTON_WIDTH_HINT, SWT.DEFAULT));
		buttonAdd.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addParameter();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		buttonEdit = new Button(buttonArea, SWT.PUSH);
		buttonEdit.setText(Messages.TrapConfigurationDialog_Edit);
		buttonEdit.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editParameter();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		buttonDelete = new Button(buttonArea, SWT.PUSH);
		buttonDelete.setText(Messages.TrapConfigurationDialog_Delete);
		buttonDelete.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteParameters();
			}
		});
		
		buttonUp = new Button(buttonArea, SWT.PUSH);
		buttonUp.setText(Messages.TrapConfigurationDialog_MoveUp);
		
		buttonDown = new Button(buttonArea, SWT.PUSH);
		buttonDown.setText(Messages.TrapConfigurationDialog_MoveDown);
		
		return dialogArea;
	}

	/**
	 * Select OID using MIB selection dialog
	 */
	private void selectObjectId()
	{
		SnmpObjectId id;
		try
		{
			id = SnmpObjectId.parseSnmpObjectId(oid.getText());
		}
		catch(SnmpObjectIdFormatException e)
		{
			id = null;
		}
		MibSelectionDialog dlg = new MibSelectionDialog(getShell(), id);
		if (dlg.open() == Window.OK)
		{
			oid.setText(dlg.getSelectedObject().getObjectId().toString());
			oid.setFocus();
		}
	}

	/**
	 * Add new parameter mapping
	 */
	private void addParameter()
	{
		SnmpTrapParameterMapping pm = new SnmpTrapParameterMapping(new SnmpObjectId());
		ParamMappingEditDialog dlg = new ParamMappingEditDialog(getShell(), pm);
		if (dlg.open() == Window.OK)
		{
			pmap.add(pm);
			paramList.setInput(pmap.toArray());
			paramList.setSelection(new StructuredSelection(pm));
		}
	}
	
	/**
	 * Edit currently selected parameter mapping
	 */
	private void editParameter()
	{
		IStructuredSelection selection = (IStructuredSelection)paramList.getSelection();
		if (selection.size() != 1)
			return;
		
		SnmpTrapParameterMapping pm = (SnmpTrapParameterMapping)selection.getFirstElement();
		ParamMappingEditDialog dlg = new ParamMappingEditDialog(getShell(), pm);
		if (dlg.open() == Window.OK)
		{
			paramList.update(pm, null);
		}
	}

	/**
	 * Delete selected parameters
	 */
	@SuppressWarnings("unchecked")
	private void deleteParameters()
	{
		IStructuredSelection selection = (IStructuredSelection)paramList.getSelection();
		if (selection.isEmpty())
			return;
		
		Iterator<SnmpTrapParameterMapping> it = selection.iterator();
		while(it.hasNext())
			pmap.remove(it.next());
		
		paramList.setInput(pmap.toArray());
	}

	/**
	 * Setup parameter mapping list
	 */
	private void setupParameterList()
	{
		Table table = paramList.getTable();
		table.setHeaderVisible(true);
		
		TableColumn tc = new TableColumn(table, SWT.LEFT);
		tc.setText(Messages.TrapConfigurationDialog_Number);
		tc.setWidth(90);
		
		tc = new TableColumn(table, SWT.LEFT);
		tc.setText(Messages.TrapConfigurationDialog_Parameter);
		tc.setWidth(200);
		
		pmap = new ArrayList<SnmpTrapParameterMapping>(trap.getParameterMapping());
		
		paramList.setContentProvider(new ArrayContentProvider());
		paramList.setLabelProvider(new ParamMappingLabelProvider(pmap));
		paramList.setInput(pmap.toArray());
		
		WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), PARAMLIST_TABLE_SETTINGS);
		
		paramList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editParameter();
			}
		});
	}
	
	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		WidgetHelper.saveColumnSettings(paramList.getTable(), Activator.getDefault().getDialogSettings(), PARAMLIST_TABLE_SETTINGS);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			trap.setObjectId(SnmpObjectId.parseSnmpObjectId(oid.getText()));
		}
		catch(SnmpObjectIdFormatException e)
		{
			MessageDialog.openWarning(getShell(), Messages.TrapConfigurationDialog_Warning, Messages.TrapConfigurationDialog_WarningInvalidOID);
			return;
		}

		trap.setDescription(description.getText());
		trap.setEventCode((int)event.getEventCode());
		trap.setUserTag(eventTag.getText());
		trap.getParameterMapping().clear();
		trap.getParameterMapping().addAll(pmap);
		
		saveSettings();
		super.okPressed();
	}
}
