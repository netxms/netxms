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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.helpers.SnmpTrapComparator;
import org.netxms.ui.eclipse.serverconfig.dialogs.helpers.TrapListLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Select SNMP traps
 */
public class SelectSnmpTrapDialog extends Dialog
{
	public static final int COLUMN_OID = 0;
	public static final int COLUMN_EVENT = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
	private List<SnmpTrap> trapList;
	private SortableTableViewer viewer;
	private List<SnmpTrap> selection;
	
	/**
	 * @param parentShell
	 * @param trapList
	 */
	public SelectSnmpTrapDialog(Shell parentShell, List<SnmpTrap> trapList)
	{
		super(parentShell);
		this.trapList = trapList;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().SelectSnmpTrapDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		final Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);
		
		final String[] names = { Messages.get().SelectSnmpTrapDialog_ColOID, Messages.get().SelectSnmpTrapDialog_ColEvent, Messages.get().SelectSnmpTrapDialog_ColDescription };
		final int[] widths = { 350, 250, 400 };
		viewer = new SortableTableViewer(dialogArea, names, widths, COLUMN_OID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new TrapListLabelProvider());
		viewer.setComparator(new SnmpTrapComparator());
		viewer.setInput(trapList.toArray());
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 450;
		viewer.getControl().setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		selection = ((IStructuredSelection)viewer.getSelection()).toList();
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public List<SnmpTrap> getSelection()
	{
		return selection;
	}
}
