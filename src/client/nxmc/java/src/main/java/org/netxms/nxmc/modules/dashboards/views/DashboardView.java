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
package org.netxms.nxmc.modules.dashboards.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardModifyListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Dashboard view
 */
public class DashboardView extends AbstractDashboardView
{
   private final I18n i18n = LocalizationHelper.getI18n(DashboardView.class);   

   private DashboardModifyListener dbcModifyListener;
   private Action actionEditMode;
   private Action actionAddColumn;
   private Action actionRemoveColumn;
   private Action actionSave;
   private boolean readOnly;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public DashboardView()
   {
      super(LocalizationHelper.getI18n(DashboardView.class).tr("Dashboard"), ResourceManager.getImageDescriptor("icons/object-views/dashboard.png"), "DashboardView");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Dashboard);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            dbc.saveDashboard();
         }
      };
      actionSave.setEnabled(false);

      actionEditMode = new Action(i18n.tr("&Edit mode"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            dbc.setEditMode(!dbc.isEditMode());
            actionEditMode.setChecked(dbc.isEditMode());
            if (!dbc.isEditMode())
               rebuildDashboard((Dashboard)getObject(), null);
         }
      };
      actionEditMode.setImageDescriptor(SharedIcons.EDIT);

      actionAddColumn = new Action("Add &column", ResourceManager.getImageDescriptor("icons/add-column.png")) {
         @Override
         public void run()
         {
            dbc.addColumn();
            if (dbc.getColumnCount() == 128)
               actionAddColumn.setEnabled(false);
            actionRemoveColumn.setEnabled(true);
         }
      };

      actionRemoveColumn = new Action("&Remove column", ResourceManager.getImageDescriptor("icons/remove-column.png")) {
         @Override
         public void run()
         {
            dbc.removeColumn();
            if (dbc.getColumnCount() == dbc.getMinimalColumnCount())
               actionRemoveColumn.setEnabled(false);
            actionAddColumn.setEnabled(true);
         }
      };
      
      dbcModifyListener = new DashboardModifyListener() {
         @Override
         public void save()
         {
            actionSave.setEnabled(false);
         }

         @Override
         public void modify()
         {
            actionSave.setEnabled(true);
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionEditMode);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionAddColumn);
      manager.add(actionRemoveColumn);
      manager.add(new Separator());
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionEditMode);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionAddColumn);
      manager.add(actionRemoveColumn);
      manager.add(new Separator());
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {      
      readOnly = ((object.getEffectiveRights() & UserAccessRights.OBJECT_ACCESS_MODIFY) == 0);
      rebuildDashboard((Dashboard)object, null);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#rebuildDashboard(org.netxms.client.objects.Dashboard, org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void rebuildDashboard(Dashboard dashboard, AbstractObject dashboardContext)
   {
      super.rebuildDashboard(dashboard, dashboardContext);
      if ((dbc != null) && !readOnly)
      {
         actionAddColumn.setEnabled(dbc.getColumnCount() < 128);
         actionRemoveColumn.setEnabled(dbc.getColumnCount() != dbc.getMinimalColumnCount());
         actionSave.setEnabled(dbc.isModified());
         dbc.setModifyListener(dbcModifyListener);
         actionEditMode.setEnabled(true);
      }
      else
      {
         actionAddColumn.setEnabled(false);
         actionRemoveColumn.setEnabled(false);
         actionSave.setEnabled(false);
         actionEditMode.setEnabled(false);
      }
   }   
}
