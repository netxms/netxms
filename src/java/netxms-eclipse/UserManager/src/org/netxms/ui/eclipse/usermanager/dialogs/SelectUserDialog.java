/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.UserListFilter;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.UserListLabelProvider;

/**
 * User selection dialog
 *
 */
public class SelectUserDialog extends Dialog
{
	private Text filterText;
	private TableViewer userList;
	private NXCSession session;
	private Class<? extends AbstractUserObject> classFilter;
	private boolean multiSelection = true;
	private AbstractUserObject[] selection;
	private UserListFilter filter;
	
	/**
	 * @param parentShell
	 */
	public SelectUserDialog(Shell parentShell, Class<? extends AbstractUserObject> classFilter)
	{
		super(parentShell);
		this.classFilter = classFilter;
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
		newShell.setText(Messages.get().SelectUserDialog_Title);
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
      
      filterText = new Text(dialogArea, SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filterText.setLayoutData(gd);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilter(filterText.getText());
            userList.refresh(false);
         }
      });
		
		new Label(dialogArea, SWT.NONE).setText(Messages.get().SelectUserDialog_AvailableUsers);
		
      userList = new TableViewer(dialogArea, SWT.BORDER | (multiSelection ? SWT.MULTI : 0) | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      final UserListLabelProvider labelProvider = new UserListLabelProvider();
      userList.setLabelProvider(labelProvider);
      userList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            String s1 = labelProvider.getText(e1);
            String s2 = labelProvider.getText(e2);
            return s1.compareToIgnoreCase(s2);
         }
      });
      userList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
			   if (getButton(IDialogConstants.OK_ID).isEnabled())
			      SelectUserDialog.this.okPressed();
			}
      });
      userList.addFilter(new ViewerFilter() {
			@Override
			public boolean select(Viewer viewer, Object parentElement, Object element)
			{
				return classFilter.isInstance(element) || (element instanceof String);
			}
      });
      userList.setInput(session.getUserDatabaseObjects());
      
      filter = new UserListFilter();
      userList.addFilter(filter);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 600;
      userList.getControl().setLayoutData(gd);
      
      return dialogArea;
	}
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Control content = super.createContents(parent);
      // must be called from createContents to make sure that buttons already created
      getUsersAndRefresh();
      return content;
   }

   /**
    * Get user info and refresh view
    */
   private void getUsersAndRefresh()
   {   
      if (session.isUserDatabaseSynchronized())
         return;

      userList.setInput(new String[] { "Loading..." });
      getButton(IDialogConstants.OK_ID).setEnabled(false);
      
      ConsoleJob job = new ConsoleJob("Synchronize users", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.syncUserDatabase();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                      
                  userList.setInput(session.getUserDatabaseObjects());
                  getButton(IDialogConstants.OK_ID).setEnabled(true);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize users";
         }
      };
      job.setUser(false);
      job.start();  
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
			MessageDialogHelper.openWarning(getShell(), Messages.get().SelectUserDialog_Warning, Messages.get().SelectUserDialog_WarningText);
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
