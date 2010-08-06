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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.widgets.MibBrowser;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * MIB object selection dialog
 *
 */
public class MibSelectionDialog extends Dialog
{
	private static final String[] mibObjectStatus = { "", "Mandatory", "Optional", "Obsolete", "Deprecated", "Current" };
	private static final String[] mibObjectAccess = { "", "Read", "Read/Write", "Write", "None", "Notify", "Create" };
	private static final String[] mibObjectType =
	{
		"Other", "Import Item", "Object ID", "Bit String", "Integer", "Integer 32bits",
		"Integer 64bits", "Unsigned Int 32bits", "Counter", "Counter 32bits", "Counter 64bits",
		"Gauge", "Gauge 32bits", "Timeticks", "Octet String", "Opaque", "IP Address",
		"Physical Address", "Network Address", "Named Type", "Sequence ID", "Sequence",
		"Choice", "Textual Convention", "Macro", "MODCOMP", "Trap", "Notification",
		"Module ID", "NSAP Address", "Agent Capability", "Unsigned Integer", "Null",
		"Object Group", "Notification Group"
	};
	
	private MibBrowser mibTree;
	private Text oid;
	private Text description;
	private Text type;
	private Text status;
	private Text access;
	private MibObject selectedObject;
	private SnmpObjectId initialSelection;
	private boolean updateObjectId = true;
	
	/**
	 * @param parentShell
	 * @param initialSelection initial selection for MIB tree or null
	 */
	public MibSelectionDialog(Shell parentShell, SnmpObjectId initialSelection)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.initialSelection = initialSelection;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select MIB Object");
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("MibSelectionDialog.cx"), settings.getInt("MibSelectionDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
		}
	}	

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		oid = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 500, "Object identifier (OID)", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
		oid.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				onManualOidChange();
			}
		});
		
		/* MIB tree */
		Composite mibTreeGroup = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		mibTreeGroup.setLayout(layout);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		mibTreeGroup.setLayoutData(gd);
		
		Label label = new Label(mibTreeGroup, SWT.NONE);
		label.setText("MIB tree");
		
		mibTree = new MibBrowser(mibTreeGroup, SWT.BORDER);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.widthHint = 500;
		gd.heightHint = 250;
		mibTree.setLayoutData(gd);
		mibTree.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				onMibTreeSelectionChanged();
			}
		});
		
		/* MIB object information: status, type, etc. */
		Composite infoGroup = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = IUIConstants.DIALOG_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 3;
		layout.makeColumnsEqualWidth = true;
		infoGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		infoGroup.setLayoutData(gd);
		
		type = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Type", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
		status = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Status", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
		access = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Access", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		/* MIB object's description */
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		description = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.READ_ONLY,
		                                             500, "Description", "", gd);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = 150;
		description.setLayoutData(gd);
		
		if (initialSelection != null)
		{
			MibObject object = Activator.getMibTree().findObject(initialSelection, false);
			if (object != null)
			{
				mibTree.setSelection(object);
			}
		}
		
		return dialogArea;
	}

	/**
	 * Handler for selection change in MIB tree
	 */
	private void onMibTreeSelectionChanged()
	{
		MibObject object = mibTree.getSelection();
		if (object != null)
		{
			if (updateObjectId)
			{
				SnmpObjectId objectId = object.getObjectId(); 
				oid.setText((objectId != null) ? objectId.toString() : "");
			}
			description.setText(object.getDescription());
			type.setText(safeGetText(mibObjectType, object.getType(), "Unknown"));
			status.setText(safeGetText(mibObjectStatus, object.getStatus(), "Unknown"));
			access.setText(safeGetText(mibObjectAccess, object.getAccess(), "Unknown"));
		}
		else
		{
			oid.setText("");
			description.setText("");
			type.setText("");
			status.setText("");
			access.setText("");
		}
	}
	
	/**
	 * Get text for code safely.
	 * 
	 * @param texts Array with texts
	 * @param code Code to get text for
	 * @param defaultValue Default value
	 * @return text for given code or default value in case of error
	 */
	private String safeGetText(String[] texts, int code, String defaultValue)
	{
		String result;
		try
		{
			result = texts[code];
		}
		catch(Exception e)
		{
			result = defaultValue;
		}
		return result;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		selectedObject = mibTree.getSelection();
		if (selectedObject == null)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select MIB object before pressing OK");
			return;
		}
		saveSettings();
		super.okPressed();
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

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("MibSelectionDialog.cx", size.x); //$NON-NLS-1$
		settings.put("MibSelectionDialog.cy", size.y); //$NON-NLS-1$
	}

	/**
	 * @return the selectedObject
	 */
	public MibObject getSelectedObject()
	{
		return selectedObject;
	}
	
	/**
	 * Handler for manual OID change
	 */
	private void onManualOidChange()
	{
		try
		{
			SnmpObjectId id = SnmpObjectId.parseSnmpObjectId(oid.getText());
			MibObject o = Activator.getMibTree().findObject(id, false);
			if (o != null)
			{
				updateObjectId = false;
				mibTree.setSelection(o);
				updateObjectId = true;
			}
		}
		catch(SnmpObjectIdFormatException e)
		{
		}
	}
}
