/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.users.propertypages;

import java.util.HashMap;
import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
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
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.modules.users.views.helpers.BaseUserLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Group Membership" page for user
 */
public class GroupMembership extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(GroupMembership.class);
   
   private TableViewer groupList;
	private NXCSession session;
	private User object;
   private HashMap<Integer, AbstractUserObject> groups = new HashMap<Integer, AbstractUserObject>(0);

   /**
    * Default constructor
    */
   public GroupMembership(User user, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(GroupMembership.class).tr("Group Membership"), messageArea);
      session = Registry.getSession();
      object = user;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{		
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      groupList = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		groupList.setContentProvider(new ArrayContentProvider());
		groupList.setLabelProvider(new BaseUserLabelProvider());
      groupList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((AbstractUserObject)e1).getName().compareToIgnoreCase(((AbstractUserObject)e2).getName());
         }
      });

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      groupList.getControl().setLayoutData(gd);

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);

      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				UserSelectionDialog dlg = new UserSelectionDialog(GroupMembership.this.getShell(), UserGroup.class);
				if (dlg.open() == Window.OK)
				{
					AbstractUserObject[] selection = dlg.getSelection();
					for(AbstractUserObject user : selection)
						groups.put(user.getId(), user);
					groupList.setInput(groups.values().toArray(new AbstractUserObject[groups.size()]));
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
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
            IStructuredSelection selection = groupList.getStructuredSelection();
				Iterator<AbstractUserObject> it = selection.iterator();
				while(it.hasNext())
				{
					AbstractUserObject element = it.next();
               if (element.getId() != AbstractUserObject.WELL_KNOWN_ID_EVERYONE)
                  groups.remove(element.getId());
				}
				groupList.setInput(groups.values().toArray());
			}
      });
		
      groupList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = groupList.getStructuredSelection();
            deleteButton.setEnabled(!selection.isEmpty() && !((selection.size() == 1) && (((AbstractUserObject)selection.getFirstElement()).getId() == AbstractUserObject.WELL_KNOWN_ID_EVERYONE)));
			}
      });

      // Initial data
      for(int groupId : object.getGroups())
		{
			final AbstractUserObject group = session.findUserDBObjectById(groupId, null);
			if (group != null)
			{
				groups.put(group.getId(), group);
			}
		}
      final AbstractUserObject group = session.findUserDBObjectById(AbstractUserObject.WELL_KNOWN_ID_EVERYONE, null);
      if (group != null)
      {
         groups.put(group.getId(), group);
      }
		groupList.setInput(groups.values().toArray());

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
      if (isApply)
      {
         setMessage(null);
         setValid(false);
      }

      int[] groupIds = new int[groups.size()];
		int i = 0;
      for(Integer id : groups.keySet())
			groupIds[i++] = id;
		object.setGroups(groupIds);

      new Job(i18n.tr("Update user database object"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_GROUP_MEMBERSHIP);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update user object");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> GroupMembership.this.setValid(true));
			}
		}.start();
		return true;
	}
}
