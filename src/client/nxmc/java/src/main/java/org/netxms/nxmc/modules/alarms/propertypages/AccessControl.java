/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2024 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.propertypages;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.editors.AlarmCategoryEditor;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.modules.users.views.helpers.BaseUserLabelProvider;
import org.netxms.nxmc.modules.users.views.helpers.UserComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Acces Control" property page of an Alarm Category
 */
public class AccessControl extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AccessControl.class);
   
   private SortableTableViewer userList;
   private NXCSession session;
   private AlarmCategory category;
   private AlarmCategoryEditor editor;
   private HashMap<Integer, AbstractUserObject> accessMap = new HashMap<Integer, AbstractUserObject>(0);
   
   
   /**
    * Constructor
    * 
    * @param editor editor for alarm catogory 
    */
   public AccessControl(AlarmCategoryEditor editor)
   {
      super(LocalizationHelper.getI18n(AccessControl.class).tr("Acces Control"));
      this.editor = editor;
      category = editor.getObjectAsItem();
   }

   @Override
   protected Control createContents(Composite parent)
   {
      session = Registry.getSession();
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { i18n.tr("Login Name") };
      final int[] columnWidths = { 300 };
      userList = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new BaseUserLabelProvider());
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
      addButton.setText(i18n.tr("Add..."));
      addButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            UserSelectionDialog dlg = new UserSelectionDialog(AccessControl.this.getShell(), AbstractUserObject.class);
            if (dlg.open() == Window.OK)
            {
               AbstractUserObject[] selection = dlg.getSelection();
               for(AbstractUserObject user : selection)
                  accessMap.put(user.getId(), user);
               userList.setInput(accessMap.values().toArray(new AbstractUserObject[accessMap.size()]));
            }
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
         
      });
      
      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("Delete"));
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
            IStructuredSelection selecion = (IStructuredSelection)userList.getSelection();
            Iterator<AbstractUserObject> iterator = selecion.iterator();
            while(iterator.hasNext())
            {
               AbstractUserObject element = iterator.next();
               accessMap.remove(element.getId());
            }
            userList.setInput(accessMap.values().toArray());
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
      for(int userId : category.getAccessControl())
      {
         final AbstractUserObject user = session.findUserDBObjectById(userId, null);
         if (user != null)
         {
            accessMap.put(user.getId(), user);
         }
      }
      userList.setInput(accessMap.values().toArray());
      
      getUsersAndRefresh();
      
      return dialogArea;
   }
   
   /**
    * Get user info and refresh view
    */
   private void getUsersAndRefresh()
   {
      if (session.isUserDatabaseSynchronized())
      {
         return;
      }
      
      Job job = new Job(i18n.tr("Synchronize users"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (session.syncMissingUsers(new HashSet<Integer>(Arrays.asList(category.getAccessControl()))))
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {    
                     for(int userId : category.getAccessControl())
                     {
                        final AbstractUserObject user = session.findUserDBObjectById(userId, null);
                        if (user != null)
                        {
                           accessMap.put(user.getId(), user);
                        }
                     }
                     userList.setInput(accessMap.values().toArray());
                  }
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
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {      
      Integer[] accessControlIds = new Integer[accessMap.size()];
      int i = 0;
      for(Integer id : accessMap.keySet())
         accessControlIds[i++] = id;
      category.setAccessControl(accessControlIds);

      editor.modify();
      return true;
   }
}
