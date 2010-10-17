package org.netxms.ui.eclipse.usermanager.propertypages;

import java.util.HashMap;
import java.util.Iterator;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.UserGroup;
import org.netxms.api.client.users.UserManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.UserComparator;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class Members extends PropertyPage
{
	private SortableTableViewer userList;
	private UserManager userManager;
	private UserGroup object;
	private HashMap<Long, AbstractUserObject> members = new HashMap<Long, AbstractUserObject>(0);

	public Members()
	{
		userManager = (UserManager)ConsoleSharedData.getSession();
	}

	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		object = (UserGroup)getElement().getAdapter(UserGroup.class);

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      final String[] columnNames = { "Login Name" };
      final int[] columnWidths = { 300 };
      userList = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		userList.setContentProvider(new ArrayContentProvider());
		userList.setLabelProvider(new WorkbenchLabelProvider());
		userList.setComparator(new UserComparator());

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      userList.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);
      
      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("Add...");
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				SelectUserDialog dlg = new SelectUserDialog(Members.this.getShell(), false);
				if (dlg.open() == Window.OK)
				{
					AbstractUserObject[] selection = dlg.getSelection();
					for(AbstractUserObject user : selection)
						members.put(user.getId(), user);
					userList.setInput(members.values().toArray(new AbstractUserObject[members.size()]));
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("Delete");
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				Iterator<AbstractUserObject> it = sel.iterator();
				while(it.hasNext())
				{
					AbstractUserObject element = it.next();
					members.remove(element.getId());
				}
				userList.setInput(members.values().toArray());
			}
      });
		
      userList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				deleteButton.setEnabled(!userList.getSelection().isEmpty());
			}
      });

      // Initial data
		for(long userId : object.getMembers())
		{
			final AbstractUserObject user = userManager.findUserDBObjectById(userId);
			if (user != null)
			{
				members.put(user.getId(), user);
			}
		}
		userList.setInput(members.values().toArray());

		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		long[] memberIds = new long[members.size()];
		int i = 0;
		for(Long id : members.keySet())
			memberIds[i++] = id;
		object.setMembers(memberIds);
		
		new Job("Update user database object") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					userManager.modifyUserDBObject(object, UserManager.USER_MODIFY_MEMBERS);
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NetXMSClientException) ? ((NetXMSClientException)e).getErrorCode() : 0,
					                    "Cannot update user account: " + e.getMessage(), null);
				}

				if (isApply)
				{
					new UIJob("Update \"Members\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							Members.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
