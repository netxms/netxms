/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.users.dialogs;

import java.util.Arrays;
import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.dialogs.DialogWithFilter;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.helpers.UserListFilter;
import org.netxms.nxmc.modules.users.views.helpers.BaseUserLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User selection dialog
 */
public class UserSelectionDialog extends DialogWithFilter
{
   private final I18n i18n = LocalizationHelper.getI18n(UserSelectionDialog.class);

	private TableViewer userList;
	private NXCSession session;
	private Class<? extends AbstractUserObject> classFilter;
	private boolean multiSelection = true;
	private AbstractUserObject[] selection;
	
	/**
	 * @param parentShell
	 */
	public UserSelectionDialog(Shell parentShell, Class<? extends AbstractUserObject> classFilter)
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
      newShell.setText(i18n.tr("Select User"));
		super.configureShell(newShell);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
      session = Registry.getSession();

		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);  
		
      userList = new TableViewer(dialogArea, SWT.BORDER | (multiSelection ? SWT.MULTI : 0) | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      BaseUserLabelProvider labelProvider = new BaseUserLabelProvider();
      userList.setLabelProvider(labelProvider);
      userList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            String s1 = (e1 instanceof AbstractUserObject) ? ((AbstractUserObject)e1).getName() : e1.toString();
            String s2 = (e2 instanceof AbstractUserObject) ? ((AbstractUserObject)e2).getName() : e2.toString();
            return s1.compareToIgnoreCase(s2);
         }
      });
      userList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
			   if (getButton(IDialogConstants.OK_ID).isEnabled())
			      UserSelectionDialog.this.okPressed();
			}
      });

      UserListFilter filter = new UserListFilter(labelProvider);
      userList.addFilter(filter);
      userList.setInput(getFilteredUserDatabaseObjects());
      setFilterClient(userList, filter);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 600;
      userList.getControl().setLayoutData(gd);

      return dialogArea;
	}
   
   /**
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
    * Get filtered list of user database objects.
    *
    * @return filtered list of user database objects
    */
   private Object[] getFilteredUserDatabaseObjects()
   {
      return Arrays.asList(session.getUserDatabaseObjects()).stream().filter(o -> (classFilter == null) || classFilter.isInstance(o)).toArray();
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

      Job job = new Job(i18n.tr("Synchronize users"), null, null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncUserDatabase();
            final Object[] filteredList = getFilteredUserDatabaseObjects();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                      
                  userList.setInput(filteredList);
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

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
		if (sel.size() == 0)
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
               i18n.tr("You must select at least one user from list and then press OK."));
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
