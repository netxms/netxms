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
package org.netxms.ui.eclipse.usermanager.dialogs;

import java.util.Iterator;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.Session;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.UserComparator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * User selection dialog
 *
 */
public class SelectUserDialog extends Dialog
{
	private TableViewer userList;
	private Session session;
	private boolean showGroups;
	private boolean multiSelection = true;
	private AbstractUserObject[] selection;
	
	/**
	 * @param parentShell
	 */
	public SelectUserDialog(Shell parentShell, boolean showGroups)
	{
		super(parentShell);
		this.showGroups = showGroups;
	}
	
	/**
	 * @param enable
	 */
	public void enableMultiSelection(boolean enable)
	{
		multiSelection = enable;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText(Messages.SelectUserDialog_Title);
		super.configureShell(newShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		session  = ConsoleSharedData.getSession();
		
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText(Messages.SelectUserDialog_AvailableUsers);
		
      final String[] columnNames = { Messages.SelectUserDialog_LoginName };
      final int[] columnWidths = { 250 };
      userList = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | (multiSelection ? SWT.MULTI : 0) | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new WorkbenchLabelProvider());
      userList.setComparator(new UserComparator());
      userList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				SelectUserDialog.this.okPressed();
			}
      });
      userList.addFilter(new ViewerFilter() {
			@Override
			public boolean select(Viewer viewer, Object parentElement, Object element)
			{
				return showGroups || (element instanceof User);
			}
      });
      userList.setInput(((UserManager)session).getUserDatabaseObjects());
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      userList.getControl().setLayoutData(gd);
      
      return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
		if (sel.size() == 0)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.SelectUserDialog_Warning, Messages.SelectUserDialog_WarningText);
			return;
		}
		selection = new AbstractUserObject[sel.size()];
		Iterator<AbstractUserObject> it = sel.iterator();
		for(int i = 0; it.hasNext(); i++)
		{
			selection[i] = it.next();
		}
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public AbstractUserObject[] getSelection()
	{
		return selection;
	}
}
