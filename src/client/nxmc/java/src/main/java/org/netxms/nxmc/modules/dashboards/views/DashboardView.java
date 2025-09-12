/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardBase;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardModifyListener;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Dashboard view
 */
public class DashboardView extends AbstractDashboardView
{
   private final I18n i18n = LocalizationHelper.getI18n(DashboardView.class);   

   private Composite contextSelectorArea;
   private ObjectSelector contextSelector;
   private AbstractObject selectedContext;
   private DashboardModifyListener dbcModifyListener;
   private Action actionEditMode;
   private Action actionAddColumn;
   private Action actionRemoveColumn;
   private Action actionSave;
   private Action actionSelectContext;
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
      return (context != null) && (context instanceof DashboardBase);
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
               rebuildDashboard((DashboardBase)getObject(), null);
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

      actionSelectContext = new Action("Select c&ontext...") {
         @Override
         public void run()
         {
            if (contextSelector != null)
               contextSelector.initiateSelection();
         }
      };
      addKeyBinding("F4", actionSelectContext);

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
      if (getObject() instanceof Dashboard && !(((Dashboard)getObject()).isTemplateInstance()))
         manager.add(actionSelectContext);
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
      DashboardBase dashboard = (DashboardBase)object;
      if ((object instanceof Dashboard) && (((Dashboard)object).isTemplateInstance()))
         rebuildDashboard(dashboard, session.findObjectById(((Dashboard)dashboard).getForcedContextObjectId()));
      else
         rebuildDashboard(dashboard, dashboard.isShowContentSelector() ? selectedContext : null);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#rebuildCurrentDashboard()
    */
   @Override
   protected void rebuildCurrentDashboard()
   {
      DashboardBase dashboard = (DashboardBase)getObject();
      rebuildDashboard(dashboard, dashboard.isShowContentSelector() ? selectedContext : null);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#rebuildDashboard(org.netxms.client.objects.DashboardBase, org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void rebuildDashboard(DashboardBase dashboard, AbstractObject dashboardContext)
   {
      if (dashboard.isShowContentSelector())
      {
         if (contextSelectorArea == null)
         {
            contextSelectorArea = new Composite(getClientArea(), SWT.NONE);
            contextSelectorArea.moveAbove(null);

            GridLayout layout = new GridLayout();
            layout.marginHeight = 0;
            layout.marginTop = 8;
            layout.marginWidth = 8;
            contextSelectorArea.setLayout(layout);

            contextSelectorArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

            contextSelector = new ObjectSelector(contextSelectorArea, SWT.NONE, new SelectorConfigurator().setShowLabel(false).setShowClearButton(false));
            contextSelector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
            contextSelector.removeSelectionClassFilter();
            contextSelector.setObjectId((selectedContext != null) ? selectedContext.getObjectId() : 0);
            contextSelector.addModifyListener(new ModifyListener() {
               @Override
               public void modifyText(ModifyEvent e)
               {
                  selectedContext = contextSelector.getObject();
                  rebuildCurrentDashboard();
               }
            });
         }
         actionSelectContext.setEnabled(true);
      }
      else
      {
         if (contextSelectorArea != null)
         {
            contextSelectorArea.dispose();
            contextSelectorArea = null;
         }
         actionSelectContext.setEnabled(false);
      }

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
