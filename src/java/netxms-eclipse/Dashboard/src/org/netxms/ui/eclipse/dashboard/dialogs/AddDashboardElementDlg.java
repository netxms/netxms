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
import org.netxms.ui.eclipse.dashboard.Messages;

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
		elementTypeSelector.add(Messages.AddDashboardElementDlg_Label);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_LineChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_BarChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_PieChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_TubeChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_StatusChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_StatusIndicator);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_Dashboard);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_NetworkMap);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_CustomWidget);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_GeoMap);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_AlarmViewer);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_AvailabilityChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_DialChart);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_WebPage);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_BarChartForTable);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_PieChartForTable);
		elementTypeSelector.add(Messages.AddDashboardElementDlg_TubeChartForTable);
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
