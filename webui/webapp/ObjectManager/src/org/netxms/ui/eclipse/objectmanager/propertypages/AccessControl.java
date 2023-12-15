/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.stream.Collectors;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.AccessListComparator;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.AccessListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.UserSelectionDialog;
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
	private NXCSession session;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		object = (AbstractObject)getElement().getAdapter(AbstractObject.class);
      session = ConsoleSharedData.getSession();

		AccessListElement[] origAcl = object.getAccessList();
		acl = new HashMap<Long, AccessListElement>(origAcl.length);
		for(int i = 0; i < origAcl.length; i++)
			acl.put(origAcl[i].getUserId(), new AccessListElement(origAcl[i]));
      collectInheritedAccessRights(object);

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
      final int[] columnWidths = { 220, 180 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
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
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				UserSelectionDialog dlg = new UserSelectionDialog(AccessControl.this.getShell(), AbstractUserObject.class);
				if (dlg.open() == Window.OK)
				{
					AbstractUserObject[] selection = dlg.getSelection();
					for(AbstractUserObject user : selection)
               {
                  AccessListElement curr = acl.get(user.getId());
                  if ((curr == null) || curr.isInherited())
                     acl.put(user.getId(), new AccessListElement(user.getId(), 0));
               }
					userList.setInput(acl.values().toArray());
               userList.setSelection(new StructuredSelection(acl.get(selection[0].getId())));
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().AccessControl_Delete);
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @SuppressWarnings("unchecked")
         @Override
			public void widgetSelected(SelectionEvent e)
			{
            IStructuredSelection selection = userList.getStructuredSelection();
				Iterator<AccessListElement> it = selection.iterator();
				while(it.hasNext())
				{
					AccessListElement element = it.next();
               if (!element.isInherited())
                  acl.remove(element.getUserId());
				}
            collectInheritedAccessRights(object);
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
      createAccessCheck(rights, "Read agent data", UserAccessRights.OBJECT_ACCESS_READ_AGENT);
      createAccessCheck(rights, "Read SNMP data", UserAccessRights.OBJECT_ACCESS_READ_SNMP);
      createAccessCheck(rights, Messages.get().AccessControl_AccessModify, UserAccessRights.OBJECT_ACCESS_MODIFY);
      createAccessCheck(rights, "Edit comments", UserAccessRights.OBJECT_ACCESS_EDIT_COMMENTS);
      createAccessCheck(rights, "Manage responsible users", UserAccessRights.OBJECT_ACCESS_EDIT_RESP_USERS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessCreate, UserAccessRights.OBJECT_ACCESS_CREATE);
      createAccessCheck(rights, Messages.get().AccessControl_AccessDelete, UserAccessRights.OBJECT_ACCESS_DELETE);
      createAccessCheck(rights, Messages.get().AccessControl_AccessControl, UserAccessRights.OBJECT_ACCESS_CONTROL);
      createAccessCheck(rights, Messages.get().AccessControl_AccessSendEvents, UserAccessRights.OBJECT_ACCESS_SEND_EVENTS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessViewAlarms, UserAccessRights.OBJECT_ACCESS_READ_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessUpdateAlarms, UserAccessRights.OBJECT_ACCESS_UPDATE_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_AccessTermAlarms, UserAccessRights.OBJECT_ACCESS_TERM_ALARMS);
      createAccessCheck(rights, Messages.get().AccessControl_CreateTicket, UserAccessRights.OBJECT_ACCESS_CREATE_ISSUE);
      createAccessCheck(rights, Messages.get().AccessControl_AccessPushData, UserAccessRights.OBJECT_ACCESS_PUSH_DATA);
      createAccessCheck(rights, Messages.get().AccessControl_AccessAccessControl, UserAccessRights.OBJECT_ACCESS_ACL);
      createAccessCheck(rights, Messages.get().AccessControl_DownloadFiles, UserAccessRights.OBJECT_ACCESS_DOWNLOAD);
      createAccessCheck(rights, Messages.get().AccessControl_UploadFiles, UserAccessRights.OBJECT_ACCESS_UPLOAD);
      createAccessCheck(rights, Messages.get().AccessControl_ManageFiles, UserAccessRights.OBJECT_ACCESS_MANAGE_FILES);
      createAccessCheck(rights, "Configure agent", UserAccessRights.OBJECT_ACCESS_CONFIGURE_AGENT);
      createAccessCheck(rights, "Take screenshot", UserAccessRights.OBJECT_ACCESS_SCREENSHOT);
      createAccessCheck(rights, "Control maintenance mode", UserAccessRights.OBJECT_ACCESS_MAINTENANCE);
      createAccessCheck(rights, "Edit maintenance journal", UserAccessRights.OBJECT_ACCESS_EDIT_MNT_JOURNAL);

      userList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection.size() == 1)
				{
               AccessListElement element = (AccessListElement)selection.getFirstElement();

               int rights = element.getAccessRights();
               for(int i = 0, mask = 1; i < accessChecks.size(); i++, mask <<= 1)
               {
                  Button check = accessChecks.get(mask);
                  if (check != null)
                  {
                     check.setSelection((rights & mask) == mask);
                  }
               }

               enableAllChecks(!element.isInherited());
				}
				else
				{
					enableAllChecks(false);
				}

            if (selection.size() > 0)
            {
               boolean allowDelete = true;
               for(Object o : selection.toList())
                  if (((AccessListElement)o).isInherited())
                  {
                     allowDelete = false;
                     break;
                  }
               deleteButton.setEnabled(allowDelete);
            }
            else
            {
               deleteButton.setEnabled(false);
            }
			}
      });

      checkInherit = new Button(dialogArea, SWT.CHECK);
      checkInherit.setText(Messages.get().AccessControl_InheritRights);
      if (object.getParentCount() > 0)
      {
         checkInherit.setSelection(object.isInheritAccessRights());
      }
      else
      {
         // For objects without parent "inherit access rights" option is meaningless
         checkInherit.setSelection(false);
         checkInherit.setEnabled(false);
      }
      checkInherit.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int oldAclSize = acl.size();

            if (checkInherit.getSelection())
            {
               collectInheritedAccessRights(object);
            }
            else
            {
               ArrayList<Long> deleteList = new ArrayList<Long>();
               for(AccessListElement element : acl.values())
                  if (element.isInherited())
                     deleteList.add(element.getUserId());
               for(Long id : deleteList)
                  acl.remove(id);
            }

            if (acl.size() != oldAclSize)
               userList.setInput(acl.values().toArray());
         }
      });

      syncUsersAndRefresh();

		return dialogArea;
	}

   /**
    * Collect inherited access rights
    */
   private void collectInheritedAccessRights(AbstractObject currentObject)
   {
      if (!currentObject.isInheritAccessRights())
         return;

      for(AbstractObject o : currentObject.getParentsAsArray())
      {
         for(AccessListElement e : o.getAccessList())
         {
            if (acl.containsKey(e.getUserId()))
               continue;
            acl.put(e.getUserId(), new AccessListElement(e, true));
         }
         collectInheritedAccessRights(o);
      }
   }

   /**
    * Get user info and refresh view
    */
   private void syncUsersAndRefresh()
   {
      if (session.isUserDatabaseSynchronized())
         return;

      ConsoleJob job = new ConsoleJob("Synchronize users", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (session.syncMissingUsers(acl.keySet()))
            {
               runInUIThread(() -> userList.refresh());
            }
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
      check.addSelectionListener(new SelectionAdapter() {
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
    * Apply changes.
    *
    * @param isApply true if update operation caused by "Apply" button
    */
   private boolean applyChanges(final boolean isApply)
   {
      boolean inheritAccessRights = checkInherit.getSelection();
      List<AccessListElement> accessRights = acl.values().stream().filter(e -> !e.isInherited()).collect(Collectors.toList());

      if (!inheritAccessRights && accessRights.isEmpty())
      {
         if (!MessageDialogHelper.openQuestion(getShell(), "Access Control Warning",
               "There are no direct or inherited access control rules for this object. If you continue with this change, object will be accessible only by the user \"system\". Do you want to proceed?"))
         {
            checkInherit.setSelection(true);
            collectInheritedAccessRights(object);
            userList.setInput(acl.values().toArray());
            return false;
         }
      }

      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setACL(accessRights);
      md.setInheritAccessRights(inheritAccessRights);

      new ConsoleJob(String.format(Messages.get().AccessControl_JobName, object.getObjectName()), null, Activator.PLUGIN_ID) {
         @Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> AccessControl.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AccessControl_JobError;
			}
		}.start();
      return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }

   /**
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
