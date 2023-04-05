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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.helpers.SnmpTrapFilter;
import org.netxms.nxmc.modules.serverconfig.dialogs.helpers.TrapListLabelProvider;
import org.netxms.nxmc.modules.snmp.views.helpers.SnmpTrapComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Select SNMP traps
 */
public class SelectSnmpTrapDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectSnmpTrapDialog.class);
   
	public static final int COLUMN_OID = 0;
	public static final int COLUMN_EVENT = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
	private List<SnmpTrap> trapList;
	private SortableTableViewer viewer;
	private List<SnmpTrap> selection;
	
	private FilterText filterText;
	private SnmpTrapFilter filter;
	
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
		newShell.setText(i18n.tr("Select Trap Mapping"));
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
		
		filterText = new FilterText(dialogArea, SWT.NONE, null, false);
		GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);
      
		final String[] names = { i18n.tr("SNMP OID"), i18n.tr("Event"), i18n.tr("Description") };
		final int[] widths = { 350, 250, 400 };
		viewer = new SortableTableViewer(dialogArea, names, widths, COLUMN_OID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new TrapListLabelProvider());
		viewer.setComparator(new SnmpTrapComparator());
		viewer.setInput(trapList.toArray());
		
		filter = new SnmpTrapFilter();
		viewer.addFilter(filter);
		
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 450;
		viewer.getControl().setLayoutData(gd);
		
		filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilterString(filterText.getText());
            viewer.refresh();
         }
      });
		
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
