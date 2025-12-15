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

import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Drill-down dashboard view
 */
public class DrilldownDashboardView extends AbstractDashboardView
{
   private final I18n i18n = LocalizationHelper.getI18n(DrilldownDashboardView.class);

   private Dashboard dashboard;
   private SessionListener clientListener;
   private long dashboardContextId; // dashboard context
   private long viewContextId; // view context

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public DrilldownDashboardView(Dashboard dashboard, long dashboardContextId, long viewContextId)
   {
      super(dashboard.getObjectName(), ResourceManager.getImageDescriptor("icons/object-views/dashboard.png"), "objects.context.dashboard." + dashboard.getObjectId());
      this.dashboard = dashboard;
      this.dashboardContextId = dashboardContextId;
      this.viewContextId = viewContextId;
   }

   /**
    * Clone constructor
    */
   protected DrilldownDashboardView()
   {
      super(LocalizationHelper.getI18n(DrilldownDashboardView.class).tr("Dashboard"), ResourceManager.getImageDescriptor("icons/object-views/dashboard.png"), null);
   } 

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      DrilldownDashboardView view = (DrilldownDashboardView)super.cloneView();
      view.dashboard = dashboard;
      view.dashboardContextId = dashboardContextId;
      view.viewContextId = viewContextId;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      rebuildCurrentDashboard();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context == null) || ((context instanceof AbstractObject) && ((AbstractObject)context).getObjectId() == viewContextId);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      clientListener = new SessionListener() {         
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED && dashboard.getObjectId() == n.getSubCode())
            {
               getViewArea().getDisplay().asyncExec(() -> {
                  dashboard = (Dashboard)n.getObject();
                  setName(dashboard.getObjectName());
                  rebuildCurrentDashboard();
               });
            }
         }
      };

      session.addListener(clientListener);
      rebuildCurrentDashboard();
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#rebuildCurrentDashboard()
    */
   @Override
   protected void rebuildCurrentDashboard()
   {
      AbstractObject contextObject = (dashboardContextId != 0) ? session.findObjectById(dashboardContextId) : null;
      rebuildDashboard(dashboard, contextObject);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(clientListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      if (dashboardContextId == 0)
         return getName();
      return getName() + " - " + session.getObjectName(dashboardContextId);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      memento.set("dashboard", dashboard.getObjectId());
      super.saveState(memento);
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      dashboard = session.findObjectById(memento.getAsLong("dashboard", 0), Dashboard.class);
      if (dashboard == null)
         throw(new ViewNotRestoredException(i18n.tr("Invalid dashboard id")));
      setName(dashboard.getObjectName());
      super.restoreState(memento);
   }
}
