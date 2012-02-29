/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.policymanager.dialogs;

import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for selecting policy installation target
 */
public class SelectInstallTargetDialog extends Dialog
{
	public static final int INSTALL_ON_CURRENT = 0;
	public static final int INSTALL_ON_SELECTED = 1;
	
	private Button radioInstallOnCurrent;
	private Button radioInstallOnSelected;
	private ObjectTree objectTree;
	private int installMode;
	private GenericObject[] selectedObjects;
	
	/**
	 * @param parentShell
	 */
	public SelectInstallTargetDialog(Shell parentShell, int initialMode)
	{
		super(parentShell);
		installMode = initialMode;
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Policy Installation Target");
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
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);

		final SelectionListener listener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				objectTree.setEnabled(radioInstallOnSelected.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};

		radioInstallOnCurrent = new Button(dialogArea, SWT.RADIO);
		radioInstallOnCurrent.setText("Install on all nodes where this policy already installed");
		radioInstallOnCurrent.setSelection(installMode == INSTALL_ON_CURRENT);
		radioInstallOnCurrent.addSelectionListener(listener);
		
		radioInstallOnSelected = new Button(dialogArea, SWT.RADIO);
		radioInstallOnSelected.setText("Install on nodes selected below");
		radioInstallOnSelected.setSelection(installMode == INSTALL_ON_SELECTED);
		radioInstallOnSelected.addSelectionListener(listener);
		
		// Read custom root objects
		long[] rootObjects = null;
		Object value = ConsoleSharedData.getProperty("PolicyManager.rootObjects");
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		objectTree = new ObjectTree(dialogArea, SWT.BORDER, ObjectTree.CHECKBOXES, rootObjects, ObjectSelectionDialog.createNodeSelectionFilter());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 300;
		objectTree.setLayoutData(gd);
		objectTree.setEnabled(installMode == INSTALL_ON_SELECTED);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		installMode = radioInstallOnCurrent.getSelection() ? INSTALL_ON_CURRENT : INSTALL_ON_SELECTED;
				
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		List<GenericObject> objects = session.findMultipleObjects(objectTree.getCheckedObjects(), false);
		Iterator<GenericObject> it = objects.iterator();
		while(it.hasNext())
		{
			GenericObject o = it.next();
			if (!(o instanceof Node))
				it.remove();
		}
		selectedObjects = objects.toArray(new GenericObject[0]);
		super.okPressed();
	}

	/**
	 * @return the installMode
	 */
	public int getInstallMode()
	{
		return installMode;
	}

	/**
	 * @return the selectedObjects
	 */
	public GenericObject[] getSelectedObjects()
	{
		return selectedObjects;
	}

}
