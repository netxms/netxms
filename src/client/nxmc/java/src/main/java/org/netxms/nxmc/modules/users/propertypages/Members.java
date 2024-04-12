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
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
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
 * "Members" page for user group
 */
public class Members extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Members.class);

   private TableViewer userList;
	private NXCSession session;
	private UserGroup object;
   private HashMap<Integer, AbstractUserObject> members = new HashMap<Integer, AbstractUserObject>(0);
   
   /**
    * Default constructor
    */
   public Members(UserGroup user, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(Members.class).tr("Members"), messageArea);
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

      if (object.getId() == AbstractUserObject.WELL_KNOWN_ID_EVERYONE)
      {
         GridLayout layout = new GridLayout();
         dialogArea.setLayout(layout);
         Label label = new Label(dialogArea, SWT.CENTER | SWT.WRAP);
         label.setText("This built-in group contains all the users in the system and\nis populated automatically. You can’t add or remove users here.");
         label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));
         return dialogArea;
      }

		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      userList = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		userList.setContentProvider(new ArrayContentProvider());
		userList.setLabelProvider(new BaseUserLabelProvider());
      userList.setComparator(new ViewerComparator() {
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
				UserSelectionDialog dlg = new UserSelectionDialog(Members.this.getShell(), AbstractUserObject.class);
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
      for(int userId : object.getMembers())
		{
			final AbstractUserObject user = session.findUserDBObjectById(userId, null);
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
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
      if (isApply)
      {
         setMessage(null);
         setValid(false);
      }

      int[] memberIds = new int[members.size()];
		int i = 0;
      for(Integer id : members.keySet())
			memberIds[i++] = id;
		object.setMembers(memberIds);

      new Job(i18n.tr("Update user database object"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_MEMBERS);
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
               runInUIThread(() -> Members.this.setValid(true));
			}
		}.start();
		return true;
	}
}
