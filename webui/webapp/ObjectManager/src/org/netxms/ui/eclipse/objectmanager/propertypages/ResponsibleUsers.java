/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.ResponsibleUser;
import org.netxms.client.users.UserGroup;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.UserSelectionDialog;

/**
 * Object`s "responsible users" property page
 */
public class ResponsibleUsers extends PropertyPage
{
   private AbstractObject object;
   private TableViewer viewer;
   private Map<Long, ResponsibleUserInfo> users = new HashMap<>();
   private NXCSession session = ConsoleSharedData.getSession();

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      object = (AbstractObject)getElement().getAdapter(AbstractObject.class);      
      for(ResponsibleUser r : object.getResponsibleUsers())
         users.put(r.userId, new ResponsibleUserInfo(r));

      // Initiate loading of user manager plugin if it was not loaded before
      Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$

      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new GridLayout());

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ResponsibleUsersLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((ResponsibleUserInfo)e1).getName().compareToIgnoreCase(((ResponsibleUserInfo)e2).getName());
         }
      });
      viewer.getTable().setHeaderVisible(true);

      TableColumn column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText("User");
      column.setWidth(300);
      column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText("Tag");
      column.setWidth(120);

      final List<String> tags = new ArrayList<String>(session.getResponsibleUserTags());
      tags.add("");
      tags.sort((s1, s2) -> s1.compareToIgnoreCase(s2));

      viewer.setColumnProperties(new String[] { "name", "tag" }); //$NON-NLS-1$ //$NON-NLS-2$
      viewer.setCellEditors(new CellEditor[] { null, new ComboBoxCellEditor(viewer.getTable(), tags.toArray(new String[tags.size()]), SWT.READ_ONLY | SWT.DROP_DOWN) });
      viewer.setCellModifier(new ICellModifier() {
         @Override
         public void modify(Object element, String property, Object value)
         {
            if (element instanceof Item)
               element = ((Item)element).getData();
            if (property.equals("tag"))
            {
               int index = (Integer)value;
               if (index >= 0)
               {
                  ((ResponsibleUserInfo)element).tag = tags.get(index);
                  viewer.update(element, new String[] { property });
               }
            }
         }

         @Override
         public Object getValue(Object element, String property)
         {
            return tags.indexOf(((ResponsibleUserInfo)element).tag);
         }

         @Override
         public boolean canModify(Object element, String property)
         {
            return property.equals("tag");
         }
      });

      viewer.setInput(users.values().toArray());

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      viewer.getControl().setLayoutData(gd);

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
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            UserSelectionDialog dlg = new UserSelectionDialog(getShell(), AbstractUserObject.class);
            if (dlg.open() == Window.OK)
            {
               AbstractUserObject[] selection = dlg.getSelection();
               for(AbstractUserObject user : selection)
                  users.put(user.getId(), new ResponsibleUserInfo(user));
               viewer.setInput(users.values().toArray());
            }
         }
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("Delete");
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            for(Object o : selection.toList())
            {
               users.remove(((ResponsibleUserInfo)o).userId);
            }
            viewer.setInput(users.values().toArray());
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            deleteButton.setEnabled(!event.getSelection().isEmpty());
         }
      });

      syncUsersAndRefresh();
      return dialogArea;
   }

   /**
    * Synchronize users and refresh view
    */
   void syncUsersAndRefresh()
   {
      if (session.isUserDatabaseSynchronized())
         return;

      ConsoleJob job = new ConsoleJob("Synchronize users", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            HashSet<Long> uids = new HashSet<Long>();
            for(ResponsibleUser r : object.getResponsibleUsers())
               uids.add(r.userId);
            if (session.syncMissingUsers(uids))
            {
               runInUIThread(() -> {
                  users.clear();
                  for(ResponsibleUser r : object.getResponsibleUsers())
                     users.put(r.userId, new ResponsibleUserInfo(r));
                  viewer.setInput(users.values().toArray());
               });
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
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      final List<ResponsibleUser> list = new ArrayList<ResponsibleUser>(users.size());
      for(ResponsibleUserInfo ri : users.values())
         list.add(new ResponsibleUser(ri.userId, ri.tag));

      new ConsoleJob(String.format(Messages.get().AccessControl_JobName, object.getObjectName()), null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updateResponsibleUsers(object.getObjectId(), list);
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> ResponsibleUsers.this.setValid(true));
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().AccessControl_JobError;
         }
      }.start();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
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
      users.clear();
      viewer.setInput(new AbstractUserObject[0]);
   }

   /**
    * Local information for responsible user
    */
   private class ResponsibleUserInfo
   {
      long userId;
      AbstractUserObject user;
      String tag;

      ResponsibleUserInfo(ResponsibleUser r)
      {
         userId = r.userId;
         user = session.findUserDBObjectById(userId, null);
         tag = r.tag;
      }

      public ResponsibleUserInfo(AbstractUserObject user)
      {
         userId = user.getId();
         this.user = user;
         tag = "";
      }

      public String getName()
      {
         return (user != null) ? user.getName() : "[" + userId + "]";
      }

      public String getLabel()
      {
         return (user != null) ? user.getLabel() : "[" + userId + "]";
      }
   }

   /**
    * Label provider for responsible users list
    */
   private static class ResponsibleUsersLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private Image imageUser = Activator.getImageDescriptor("icons/user.png").createImage();
      private Image imageGroup = Activator.getImageDescriptor("icons/group.png").createImage();

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         if (columnIndex != 0)
            return null;

         ResponsibleUserInfo r = (ResponsibleUserInfo)element;
         return (r.user != null) ? ((r.user instanceof UserGroup) ? imageGroup : imageUser) : SharedIcons.IMG_UNKNOWN_OBJECT;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         ResponsibleUserInfo r = (ResponsibleUserInfo)element;
         return (columnIndex == 0) ? r.getLabel() : r.tag;
      }

      /**
       * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         imageUser.dispose();
         imageGroup.dispose();
         super.dispose();
      }
   }
}
