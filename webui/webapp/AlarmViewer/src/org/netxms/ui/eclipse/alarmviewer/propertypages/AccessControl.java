/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.propertypages;

import java.util.HashMap;
import java.util.Iterator;
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
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.alarmviewer.editors.AlarmCategoryEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;
import org.netxms.ui.eclipse.usermanager.views.helpers.UserComparator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Acces Control" property page of an Alarm Category
 */
public class AccessControl extends PropertyPage
{
   private SortableTableViewer userList;
   private NXCSession session;
   private AlarmCategory category;
   private AlarmCategoryEditor editor;
   private HashMap<Long, AbstractUserObject> accessMap = new HashMap<Long, AbstractUserObject>(0);

   @Override
   protected Control createContents(Composite parent)
   {
      session = ConsoleSharedData.getSession();
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      editor = (AlarmCategoryEditor)getElement().getAdapter(AlarmCategoryEditor.class);
      category = editor.getObjectAsItem();
      
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
         public void widgetSelected(SelectionEvent e)
         {
            SelectUserDialog dlg = new SelectUserDialog(AccessControl.this.getShell(), User.class);
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
      for(long userId : category.getAccessControl())
      {
         final AbstractUserObject user = session.findUserDBObjectById(userId);
         if (user != null)
         {
            accessMap.put(user.getId(), user);
         }
      }
      userList.setInput(accessMap.values().toArray());
      
      return dialogArea;
   }
   
   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected boolean applyChanges(final boolean isApply)
   {      
      Long[] accessControlIds = new Long[accessMap.size()];
      int i = 0;
      for(Long id : accessMap.keySet())
         accessControlIds[i++] = id;
      category.setAccessControl(accessControlIds);
      
      editor.modify();
      return true;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
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
