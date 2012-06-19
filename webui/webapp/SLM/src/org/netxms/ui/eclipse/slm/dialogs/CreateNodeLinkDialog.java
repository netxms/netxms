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
package org.netxms.ui.eclipse.slm.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Business service object creation dialog
 *
 */
public class CreateNodeLinkDialog extends Dialog
{
	private LabeledText nameField;
	private ObjectSelector nodeField;
	
	private String name;
	private long nodeId;
	
	/**
	 * @param parentShell
	 */
	public CreateNodeLinkDialog(Shell parentShell)
	{
		super(parentShell);
	}


	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Create Node Link Object");
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
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		nameField = new LabeledText(dialogArea, SWT.NONE);
		nameField.setLabel("Name (leave empty to use node's name)");
		nameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		nameField.setLayoutData(gd);
		
		nodeField = new ObjectSelector(dialogArea, SWT.NONE);
		nodeField.setLabel("Node");
		nodeField.setObjectClass(Node.class);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		nodeField.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		nodeId = nodeField.getObjectId();
		if (nodeId == 0)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select node object to link");
			return;
		}
		
		name = nameField.getText().trim();
		if (name.isEmpty())
		{
			name = nodeField.getText();
		}
		
		super.okPressed();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}


	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}
}
