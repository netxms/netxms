/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.propertypages;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.IColorProvider;
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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.ResponsibleUser;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object`s "responsible users" property page. Shows object's own responsible users (editable) together with users inherited
 * from ancestor objects (read-only, with the object they come from). An inherited entry is hidden when the same user is
 * assigned on the object itself, matching server-side resolution where the nearest assignment wins.
 */
public class ResponsibleUsers extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ResponsibleUsers.class);

   private TableViewer viewer;
   private Map<Integer, ResponsibleUserInfo> users = new HashMap<>();
   private List<ResponsibleUserInfo> inheritedUsers = new ArrayList<>();
   private Button deleteButton;
   private NXCSession session = Registry.getSession();

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ResponsibleUsers(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ResponsibleUsers.class).tr("Responsible Users"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "responsibleUsers";
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      for(ResponsibleUser r : object.getResponsibleUsers())
         users.put(r.userId, new ResponsibleUserInfo(r, false));

      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new GridLayout());

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ResponsibleUsersLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            ResponsibleUserInfo r1 = (ResponsibleUserInfo)e1;
            ResponsibleUserInfo r2 = (ResponsibleUserInfo)e2;
            if (r1.inherited != r2.inherited)
               return r1.inherited ? 1 : -1;
            return r1.getName().compareToIgnoreCase(r2.getName());
         }
      });
      viewer.getTable().setHeaderVisible(true);

      TableColumn column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText(i18n.tr("User"));
      column.setWidth(300);
      column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText(i18n.tr("Tag"));
      column.setWidth(120);
      column = new TableColumn(viewer.getTable(), SWT.NONE);
      column.setText(i18n.tr("Inherited from"));
      column.setWidth(200);

      // Selectable tags: configured allowed tags, empty tag, and tags already in use on this object (may have been set outside allowed list)
      Set<String> tagSet = session.getResponsibleUserTags();
      tagSet.add("");
      for(ResponsibleUserInfo r : users.values())
         tagSet.add(r.tag);
      final List<String> tags = new ArrayList<String>(tagSet);
      tags.sort((s1, s2) -> s1.compareToIgnoreCase(s2));

      viewer.setColumnProperties(new String[] { "name", "tag", "source" });
      viewer.setCellEditors(new CellEditor[] { null, new ComboBoxCellEditor(viewer.getTable(), tags.toArray(new String[tags.size()]), SWT.READ_ONLY | SWT.DROP_DOWN), null });
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
            return property.equals("tag") && !((ResponsibleUserInfo)element).inherited;
         }
      });

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
      addButton.setText(i18n.tr("&Add..."));
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
               refreshViewer();
            }
         }
      });

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            for(Object o : selection.toList())
            {
               ResponsibleUserInfo r = (ResponsibleUserInfo)o;
               if (!r.inherited)
                  users.remove(r.userId);
            }
            refreshViewer();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            updateDeleteButton();
         }
      });

      refreshViewer();
      loadInheritedUsers();
      return dialogArea;
   }

   /**
    * Enable "Delete" button only when selection contains at least one of object's own entries
    */
   private void updateDeleteButton()
   {
      boolean enabled = false;
      for(Object o : viewer.getStructuredSelection().toList())
      {
         if (!((ResponsibleUserInfo)o).inherited)
         {
            enabled = true;
            break;
         }
      }
      deleteButton.setEnabled(enabled);
   }

   /**
    * Set viewer input to object's own entries followed by inherited entries not overridden locally
    */
   private void refreshViewer()
   {
      List<ResponsibleUserInfo> input = new ArrayList<>(users.size() + inheritedUsers.size());
      input.addAll(users.values());
      for(ResponsibleUserInfo r : inheritedUsers)
      {
         if (!users.containsKey(r.userId))
            input.add(r);
      }
      viewer.setInput(input.toArray());
      updateDeleteButton();
   }

   /**
    * Load effective list of responsible users from server, keep inherited entries, and make sure all referenced users are
    * available locally.
    */
   private void loadInheritedUsers()
   {
      Job job = new Job(i18n.tr("Loading inherited responsible users"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ResponsibleUser> inherited = new ArrayList<>();
            for(ResponsibleUser r : session.getEffectiveResponsibleUsers(object.getObjectId()))
            {
               if (r.sourceObjectId != object.getObjectId())
                  inherited.add(r);
            }

            if (!session.isUserDatabaseSynchronized())
            {
               HashSet<Integer> uids = new HashSet<Integer>();
               for(ResponsibleUser r : object.getResponsibleUsers())
                  uids.add(r.userId);
               for(ResponsibleUser r : inherited)
                  uids.add(r.userId);
               session.syncMissingUsers(uids);
            }

            runInUIThread(() -> {
               if (viewer.getControl().isDisposed())
                  return;
               for(ResponsibleUserInfo r : users.values())
                  r.resolveUser();
               inheritedUsers.clear();
               for(ResponsibleUser r : inherited)
                  inheritedUsers.add(new ResponsibleUserInfo(r, true));
               refreshViewer();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load inherited responsible users");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      final List<ResponsibleUser> list = new ArrayList<ResponsibleUser>(users.size());
      for(ResponsibleUserInfo ri : users.values())
         list.add(new ResponsibleUser(ri.userId, ri.tag));

      new Job(i18n.tr("Update list of responsible users for object {0}", object.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot change list of responsible users for object {0}", object.getObjectName());
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      users.clear();
      refreshViewer();
   }

   /**
    * Local information for responsible user
    */
   private class ResponsibleUserInfo
   {
      int userId;
      AbstractUserObject user;
      String tag;
      boolean inherited;
      long sourceObjectId;

      ResponsibleUserInfo(ResponsibleUser r, boolean inherited)
      {
         userId = r.userId;
         user = session.findUserDBObjectById(userId, null);
         tag = r.tag;
         this.inherited = inherited;
         sourceObjectId = r.sourceObjectId;
      }

      public ResponsibleUserInfo(AbstractUserObject user)
      {
         userId = user.getId();
         this.user = user;
         tag = "";
      }

      void resolveUser()
      {
         if (user == null)
            user = session.findUserDBObjectById(userId, null);
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
   private class ResponsibleUsersLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
   {
      private Color inheritedColor = ThemeEngine.getForegroundColor("List.DisabledItem");

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         if (columnIndex != 0)
            return null;

         ResponsibleUserInfo r = (ResponsibleUserInfo)element;
         return (r.user != null) ? ((r.user instanceof UserGroup) ? SharedIcons.IMG_GROUP : SharedIcons.IMG_USER) : SharedIcons.IMG_UNKNOWN_OBJECT;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         ResponsibleUserInfo r = (ResponsibleUserInfo)element;
         switch(columnIndex)
         {
            case 0:
               return r.getLabel();
            case 1:
               return r.tag;
            case 2:
               if (!r.inherited)
                  return "";
               return session.getObjectName(r.sourceObjectId);
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
       */
      @Override
      public Color getForeground(Object element)
      {
         return ((ResponsibleUserInfo)element).inherited ? inheritedColor : null;
      }

      /**
       * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
       */
      @Override
      public Color getBackground(Object element)
      {
         return null;
      }
   }
}
