/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.HashMap;
import java.util.Iterator;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
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
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.api.client.constants.UserAccessRights;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.AccessListComparator;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.AccessListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Object's "access control" property page
 */
public class AccessControl extends PropertyPage
{
	private AbstractObject object;
	private SortableTableViewer userList;
	private HashMap<Integer, Button> accessChecks = new HashMap<Integer, Button>(11);
	private HashMap<Long, AccessListElement> acl;
	private Button checkInherit;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		object = (AbstractObject)getElement().getAdapter(AbstractObject.class);
		
		AccessListElement[] origAcl = object.getAccessList();
		acl = new HashMap<Long, AccessListElement>(origAcl.length);
		for(int i = 0; i < origAcl.length; i++)
			acl.put(origAcl[i].getUserId(), new AccessListElement(origAcl[i]));
		
		// Initiate loading of user manager plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		Group users = new Group(dialogArea, SWT.NONE);
		users.setText(Messages.get().AccessControl_UsersGroups);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      users.setLayoutData(gd);

		layout = new GridLayout();
		users.setLayout(layout);
      
      final String[] columnNames = { Messages.get().AccessControl_ColLogin, Messages.get().AccessControl_ColRights };
      final int[] columnWidths = { 150, 100 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new AccessListLabelProvider());
      userList.setComparator(new AccessListComparator());
      userList.setInput(acl.values().toArray());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      userList.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(users, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);
      
      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().AccessControl_Add);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				SelectUserDialog dlg = new SelectUserDialog(AccessControl.this.getShell(), true);
				if (dlg.open() == Window.OK)
				{
					AbstractUserObject[] selection = dlg.getSelection();
					for(AbstractUserObject user : selection)
						acl.put(user.getId(), new AccessListElement(user.getId(), 0));
					userList.setInput(acl.values().toArray());
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().AccessControl_Delete);
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
				Iterator<AccessListElement> it = sel.iterator();
				while(it.hasNext())
				{
					AccessListElement element = it.next();
					acl.remove(element.getUserId());
				}
				userList.setInput(acl.values().toArray());
			}
      });
      
      Group rights = new Group(dialogArea, SWT.NONE);
      rights.setText(Messages.get().AccessControl_AccessRights);
      rights.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      rights.setLayoutData(gd);
      
      createAccessCheck(rights, Messages.get().AccessControl_AccessRead, UserAccessRights.OBJECT_ACCESS_READ);
      createAccessCheck(rights, Messages.get().AccessControl_AccessModify, UserAccessRights.OBJECT_ACCESS_MODIFY);
      createAccessCheck(rights, Messages.get().AccessControl_AccessCreate, UserAccessRights.OBJECT_ACCESS_CREATE);
      createAccessCheck(rights, Messages.get().AccessControl_AccessDelete, UserAccessRights.OBJECT_ACCESS_DELETE);
      createAccessCheck(rights, Messages.get().AccessControl_AccessControl, UserAccessRights.OBJECT_ACCESS_CONTROL);
      createAccessCheck(rights, Messages.get().AccessControl_AccessSendEvents, UserAccessRights.OBJECT_ACCESS_SEND_EVENTS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessViewAlarms, UserAccessRights.OBJECT_ACCESS_READ_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessAckAlarms, UserAccessRights.OBJECT_ACCESS_ACK_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessTermAlarms, UserAccessRights.OBJECT_ACCESS_TERM_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessPushData, UserAccessRights.OBJECT_ACCESS_PUSH_DATA);
      createAccessCheck(rights, Messages.get().AccessControl_AccessAccessControl, UserAccessRights.OBJECT_ACCESS_ACL);
      
      userList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection sel = (IStructuredSelection)event.getSelection();
				if (sel.size() == 1)
				{
					enableAllChecks(true);
					AccessListElement element = (AccessListElement)sel.getFirstElement();
					int rights = element.getAccessRights();
					for(int i = 0, mask = 1; i < 16; i++, mask <<= 1)
					{
						Button check = accessChecks.get(mask);
						if (check != null)
						{
							check.setSelection((rights & mask) == mask);
						}
					}
				}
				else
				{
					enableAllChecks(false);
				}
				deleteButton.setEnabled(sel.size() > 0);
			}
      });
      
      checkInherit = new Button(dialogArea, SWT.CHECK);
      checkInherit.setText(Messages.get().AccessControl_InheritRights);
      checkInherit.setSelection(object.isInheritAccessRights());
      
		return dialogArea;
	}
	
	/**
	 * Create access control check box.
	 * 
	 * @param parent Parent composite
	 * @param name Name of the access right
	 * @param bitMask Bit mask for access right
	 */
	private void createAccessCheck(final Composite parent, final String name, final Integer bitMask)
	{
      final Button check = new Button(parent, SWT.CHECK);
      check.setText(name);
      check.setEnabled(false);
      check.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				AccessListElement element = (AccessListElement)sel.getFirstElement();
				int rights = element.getAccessRights();
				if (check.getSelection())
					rights |= bitMask;
				else
					rights &= ~bitMask;
				element.setAccessRights(rights);
				userList.update(element, null);
			}
      });
      accessChecks.put(bitMask, check);
	}

	/**
	 * Enables all access check boxes if the argument is true, and disables them otherwise.
	 * 
	 * @param enabled the new enabled state
	 */
	private void enableAllChecks(boolean enabled)
	{
		for(final Button b : accessChecks.values())
		{
			b.setEnabled(enabled);
		}
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
		
		final boolean inheritAccessRights = checkInherit.getSelection();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setACL(acl.values().toArray(new AccessListElement[acl.size()]));
		md.setInheritAccessRights(inheritAccessRights);

		new ConsoleJob(String.format(Messages.get().AccessControl_JobName, object.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							AccessControl.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AccessControl_JobError;
			}
		}.start();
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		checkInherit.setSelection(true);
		acl.clear();
		userList.setInput(acl.values().toArray());
	}
}
