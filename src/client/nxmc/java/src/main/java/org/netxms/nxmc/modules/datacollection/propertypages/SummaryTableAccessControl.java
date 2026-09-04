/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.AccessListComparator;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.AccessListLabelProvider;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Access Control" property page for DCI summary table
 */
public class SummaryTableAccessControl extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryTableAccessControl.class);

   private DciSummaryTable table;
   private Set<AbstractUserObject> acl = new HashSet<AbstractUserObject>();
   private TableViewer viewer;
   private NXCSession session;

   /**
    * Create property page for given summary table
    *
    * @param table summary table to edit
    */
   public SummaryTableAccessControl(DciSummaryTable table)
   {
      super(LocalizationHelper.getI18n(SummaryTableAccessControl.class).tr("Access Control"));
      this.table = table;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      session = Registry.getSession();
      for(Integer uid : table.getAccessList())
      {
         AbstractUserObject o = session.findUserDBObjectById(uid, null);
         if (o != null)
            acl.add(o);
      }

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Users allowed to use this summary table"));

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AccessListLabelProvider());
      viewer.setComparator(new AccessListComparator());
      viewer.getTable().setSortDirection(SWT.UP);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      viewer.getTable().setLayoutData(gd);
      viewer.setInput(acl.toArray());

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttons.setLayoutData(gd);

      Button buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText(i18n.tr("Add..."));
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addUsers();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);

      Button buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText(i18n.tr("Delete"));
      buttonRemove.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeUsers();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);

      syncUsersAndRefresh();

      return dialogArea;
   }

   /**
    * Synchronize missing users and refresh viewer
    */
   private void syncUsersAndRefresh()
   {
      if (session.isUserDatabaseSynchronized())
         return;

      Job job = new Job(i18n.tr("Synchronizing users"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (session.syncMissingUsers(Arrays.asList(table.getAccessList())))
            {
               runInUIThread(() -> {
                  for(Integer uid : table.getAccessList())
                  {
                     AbstractUserObject o = session.findUserDBObjectById(uid, null);
                     if (o != null)
                        acl.add(o);
                  }
                  viewer.setInput(acl.toArray());
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize users");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Add users to access list
    */
   private void addUsers()
   {
      UserSelectionDialog dlg = new UserSelectionDialog(getShell(), AbstractUserObject.class);
      if (dlg.open() == Window.OK)
      {
         for(AbstractUserObject o : dlg.getSelection())
            acl.add(o);
         viewer.setInput(acl.toArray());
      }
   }

   /**
    * Remove users from access list
    */
   private void removeUsers()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
         acl.remove(o);
      viewer.setInput(acl.toArray());
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (!isControlCreated())
         return true;

      if (isApply)
         setValid(false);

      Integer[] accessList = new Integer[acl.size()];
      int i = 0;
      for(AbstractUserObject o : acl)
         accessList[i++] = o.getId();
      table.setAccessList(accessList);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Update DCI summary table configuration"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            synchronized(table)
            {
               int id = session.modifyDciSummaryTable(table);
               table.setId(id);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update DCI summary table configuration");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(() -> SummaryTableAccessControl.this.setValid(true));
            }
         }
      }.start();

      return true;
   }
}
