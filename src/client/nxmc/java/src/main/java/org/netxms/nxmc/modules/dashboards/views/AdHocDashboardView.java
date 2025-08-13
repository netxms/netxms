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

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Ad-hoc dashboard view
 */
public class AdHocDashboardView extends AbstractDashboardView
{
   private Dashboard dashboard;
   private AbstractObject dashboardContext;
   private long contextObjectId; // view context
   private SessionListener clientListener;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public AdHocDashboardView(long contextObjectId, Dashboard dashboard, AbstractObject dashboardContext)
   {
      super(dashboard.getObjectName(), ResourceManager.getImageDescriptor("icons/object-views/dashboard.png"), "AdHocDashboard." + contextObjectId + "." + dashboard.getObjectId());
      this.dashboard = dashboard;
      this.dashboardContext = dashboardContext;
      this.contextObjectId = contextObjectId;
   }

   /**
    * Clone constructor
    */
   protected AdHocDashboardView()
   {
      super(null, null, null);
   } 

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AdHocDashboardView view = (AdHocDashboardView)super.cloneView();
      view.dashboard = dashboard;
      view.dashboardContext = dashboardContext;
      view.contextObjectId = contextObjectId;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      long dashboardContextObjectId = (dashboardContext != null) ? dashboardContext.getObjectId() : 0;
      return (context != null) && (context instanceof AbstractObject) && 
            ((((AbstractObject)context).getObjectId() == dashboardContextObjectId) || (((AbstractObject)context).getObjectId() == contextObjectId));
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      super.createContent(parent);
      rebuildDashboard(dashboard, dashboardContext);
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
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               if (dashboard.getObjectId() == n.getSubCode())
               {
                  getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        dashboard = (Dashboard)n.getObject();
                        setName(dashboard.getObjectName());
                        rebuildDashboard(dashboard, dashboardContext);
                     }
                  });
               }
               else if ((dashboardContext != null) && (dashboardContext.getObjectId() == n.getSubCode()))
               {
                  getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        dashboardContext = (AbstractObject)n.getObject();
                        rebuildDashboard(dashboard, dashboardContext);
                     }
                  });
               }
            }
         }
      };

      session.addListener(clientListener);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView#rebuildCurrentDashboard()
    */
   @Override
   protected void rebuildCurrentDashboard()
   {
      rebuildDashboard(dashboard, dashboardContext);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      // Ignore object change - this view always show specific dashboard
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10000;
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
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(clientListener);
      super.dispose();
   }
}
