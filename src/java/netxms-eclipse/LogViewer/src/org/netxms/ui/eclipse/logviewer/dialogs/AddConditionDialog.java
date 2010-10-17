/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class AddConditionDialog extends Dialog
{
	private LogColumn column;
	private ColumnFilter filter;
	private Text likeValue;
	private ObjectSelector objectSelector;
	
	/**
	 * Create dialog
	 * 
	 * @param parentShell
	 */
	protected AddConditionDialog(Shell parentShell, LogColumn column)
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
		newShell.setText("Add condition");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		GridData gd;
		switch(column.getType())
		{
			case LogColumn.LC_OBJECT_ID:
				objectSelector = new ObjectSelector(dialogArea, SWT.NONE);
				objectSelector.setLabel("Object equals to:");
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				gd.widthHint = 250;
				objectSelector.setLayoutData(gd);
				break;
			case LogColumn.LC_USER_ID:
				break;
			default:	// Assume generic text field
				Label label = new Label(dialogArea, SWT.NONE);
				label.setText("Text equals to\n(You can use % as metacharacter to match multiple characters):");
				
				likeValue = new Text(dialogArea, SWT.BORDER);
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				likeValue.setLayoutData(gd);
				break;
		}
	
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		switch(column.getType())
		{
			case LogColumn.LC_OBJECT_ID:
				long objectId = objectSelector.getObjectId();
				if (objectId != 0)
				{
					filter = new ColumnFilter(objectId);
				}
				else
				{
					MessageDialog.openWarning(getShell(), "Warning", "Please select object and than press OK");
					return;
				}
				break;
			default:
				filter = new ColumnFilter(likeValue.getText());
				break;
		}
		super.okPressed();
	}

	/**
	 * Get resulting filter
	 * 
	 * @return filter based on user input
	 */
	public ColumnFilter getFilter()
	{
		return filter;
	}
}
