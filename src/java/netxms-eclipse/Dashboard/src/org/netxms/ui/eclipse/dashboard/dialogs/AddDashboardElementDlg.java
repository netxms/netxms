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
package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

/**
 * Dialog for adding new dashboard element
 */
public class AddDashboardElementDlg extends Dialog
{
	private Combo elementTypeSelector; 
	private int elementType;
	
	public AddDashboardElementDlg(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		elementTypeSelector = new Combo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY);
		elementTypeSelector.add("Label");
		elementTypeSelector.add("Line Chart");
		elementTypeSelector.add("Bar Chart");
		elementTypeSelector.add("Pie Chart");
		elementTypeSelector.add("Tube Chart");
		elementTypeSelector.add("Status Chart");
		elementTypeSelector.add("Status Indicator");
		elementTypeSelector.add("Dashboard");
		elementTypeSelector.add("Network Map");
		elementTypeSelector.add("Custom Widget");
		elementTypeSelector.add("Geo Map");
		elementTypeSelector.add("Alarm Viewer");
		elementTypeSelector.add("Availability Chart");
		elementTypeSelector.add("Dial Chart");
		elementTypeSelector.add("Web Page");
		elementTypeSelector.select(1);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		elementType = elementTypeSelector.getSelectionIndex();
		super.okPressed();
	}

	/**
	 * @return the elementType
	 */
	public int getElementType()
	{
		return elementType;
	}
}
