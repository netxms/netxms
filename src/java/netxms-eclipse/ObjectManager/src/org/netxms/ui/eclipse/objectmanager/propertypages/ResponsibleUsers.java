/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
import java.util.Iterator;
import java.util.List;
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
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.ResponsibleUsersComparator;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.ResponsibleUsersLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Object`s "responsible users" property page
 */
public class ResponsibleUsers extends PropertyPage
{
   private AbstractObject object;
   private SortableTableViewer userTable;
   private List<AbstractUserObject> userList;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      object = (AbstractObject)getElement().getAdapter(AbstractObject.class);      
      userList = session.findUserDBObjectsByIds(object.getResponsibleUsers());
      
      // Initiate loading of user manager plugin if it was not loaded before
      Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new GridLayout());
      
      final String[] columnNames = { "Login Name" };
      final int[] columnWidths = { 150 };
      userTable = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                          SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userTable.setContentProvider(new ArrayContentProvider());
      userTable.setLabelProvider(new ResponsibleUsersLabelProvider());
      userTable.setComparator(new ResponsibleUsersComparator());
      userTable.setInput(userList.toArray());
      
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      userTable.getControl().setLayoutData(gd);
      
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
            SelectUserDialog dlg = new SelectUserDialog(getShell(), AbstractUserObject.class);
            if (dlg.open() == Window.OK)
            {
               AbstractUserObject[] selection = dlg.getSelection();
               for(AbstractUserObject user : selection)
                  userList.add(user);
               userTable.setInput(userList.toArray());
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
            IStructuredSelection selection = (IStructuredSelection)userTable.getSelection();
            Iterator<AbstractUserObject> it = selection.iterator();
            while(it.hasNext())
            {
               AbstractUserObject element = it.next();
               userList.remove(element);
            }
            userTable.setInput(userList.toArray());
         }
      });
      
      userTable.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            deleteButton.setEnabled(!selection.isEmpty());
         }
      });
      
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
      
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      
      List<Long> userIds = new ArrayList<Long>(userList.size());
      for(AbstractUserObject o : userList)
         userIds.add(o.getId());
      md.setResponsibleUsers(userIds);

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
                     ResponsibleUsers.this.setValid(true);
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
      userList.clear();
      userTable.setInput(userList.toArray());
   }
}
