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
package org.netxms.ui.eclipse.snmp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.shared.MibCache;
import org.netxms.ui.eclipse.snmp.widgets.MibBrowser;
import org.netxms.ui.eclipse.snmp.widgets.MibObjectDetails;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * MIB object selection dialog
 *
 */
public class MibSelectionDialog extends Dialog
{
	private long nodeId = 0;
	private MibBrowser mibTree;
	private Text oid;
	private MibObjectDetails details;
	private MibObject selectedObject;
	private SnmpObjectId selectedObjectId;
	private SnmpObjectId initialSelection;
	private boolean updateObjectId = true;
	
	/**
	 * @param parentShell
	 * @param initialSelection initial selection for MIB tree or null
	 */
	public MibSelectionDialog(Shell parentShell, SnmpObjectId initialSelection, long nodeId)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.initialSelection = initialSelection;
		this.nodeId = nodeId;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.MibSelectionDialog_Title);
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
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = (nodeId != 0) ? 3 : 2;
		dialogArea.setLayout(layout);
		
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
		gd.verticalSpan = 2;
		mibTreeGroup.setLayoutData(gd);
		
		Label label = new Label(mibTreeGroup, SWT.NONE);
		label.setText(Messages.MibSelectionDialog_MIBTree);
		
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

		oid = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 500, Messages.MibSelectionDialog_OID, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
		oid.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				onManualOidChange();
			}
		});
		
		if (nodeId != 0)
		{
			Button button = new Button(dialogArea, SWT.PUSH);
			button.setText("&Walk...");
			gd = new GridData();
			gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
			gd.verticalAlignment = SWT.BOTTOM;
			button.setLayoutData(gd);
			button.addSelectionListener(new SelectionListener() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					doWalk();
				}
				
				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
		details = new MibObjectDetails(dialogArea, SWT.NONE, false, mibTree);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.horizontalSpan = (nodeId != 0) ? 2 : 1;
		details.setLayoutData(gd);
		
		if (initialSelection != null)
		{
			MibObject object = MibCache.getMibTree().findObject(initialSelection, false);
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
		if ((object != null) && updateObjectId)
		{
			SnmpObjectId objectId = object.getObjectId(); 
			oid.setText((objectId != null) ? objectId.toString() : ""); //$NON-NLS-1$
		}
		details.setObject(object);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		try
		{
			selectedObjectId = SnmpObjectId.parseSnmpObjectId(oid.getText());
		}
		catch(SnmpObjectIdFormatException e)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.MibSelectionDialog_Warning, "Please enter valid SNMP object ID or select one from the tree");
			return;
		}
		
		selectedObject = mibTree.getSelection();
		if (selectedObject == null)
		{
			selectedObject = MibCache.getMibTree().findObject(selectedObjectId, false);
			if (selectedObject == null)
			{
				MessageDialogHelper.openWarning(getShell(), Messages.MibSelectionDialog_Warning, Messages.MibSelectionDialog_WarningText);
				return;
			}
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
		MibObject o = MibCache.findObject(oid.getText(), false);
		if (o != null)
		{
			updateObjectId = false;
			mibTree.setSelection(o);
			updateObjectId = true;
		}
	}

	/**
	 * Do SNMP walk
	 */
	private void doWalk()
	{
		try
		{
			final SnmpObjectId root = SnmpObjectId.parseSnmpObjectId(oid.getText());
			MibWalkDialog dlg = new MibWalkDialog(getShell(), nodeId, root);
			if (dlg.open() == Window.OK)
			{
				SnmpValue v = dlg.getValue();
				if (v != null)
				{
					oid.setText(v.getName());
				}
			}
		}
		catch(SnmpObjectIdFormatException e)
		{
			MessageDialogHelper.openError(getShell(), "Error", "Error parsing SNMP OID");
		}
	}

	/**
	 * @return
	 */
	public SnmpObjectId getSelectedObjectId()
	{
		return selectedObjectId;
	}
}
