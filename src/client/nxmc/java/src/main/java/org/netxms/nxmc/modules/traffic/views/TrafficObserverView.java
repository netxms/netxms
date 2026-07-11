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
package org.netxms.nxmc.modules.traffic.views;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.traffic.views.helpers.ObservationPointFilter;
import org.netxms.nxmc.modules.traffic.views.helpers.ObservationPointListComparator;
import org.netxms.nxmc.modules.traffic.views.helpers.ObservationPointListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

/**
 * "Observation Points" view for traffic observer objects: connection/backend health
 * header and the list of discovered observation points.
 */
public class TrafficObserverView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(TrafficObserverView.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_EXTERNAL_ID = 2;
   public static final int COLUMN_TYPE = 3;
   public static final int COLUMN_STATE = 4;
   public static final int COLUMN_PROVIDER_STATE = 5;
   public static final int COLUMN_SAMPLING_RATE = 6;
   public static final int COLUMN_IN_SCOPE = 7;
   public static final int COLUMN_LAST_DISCOVERY = 8;
   public static final int COLUMN_STATUS = 9;

   private Label connectionStateLabel;
   private Label backendLabel;
   private Label capabilitiesLabel;
   private Label lastDiscoveryLabel;
   private Label syncStatusLabel;
   private SortableTableViewer viewer;
   private Action actionExportToCsv;
   private Action actionIncludeInScope;
   private Action actionExcludeFromScope;
   private SessionListener sessionListener;

   /**
    * Create view
    */
   public TrafficObserverView()
   {
      super(LocalizationHelper.getI18n(TrafficObserverView.class).tr("Observation Points"),
            ResourceManager.getImageDescriptor("icons/object-views/access-points.png"), "objects.observation-points", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 30;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof TrafficObserver);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      Composite header = new Composite(parent, SWT.NONE);
      GridLayout headerLayout = new GridLayout(2, false);
      header.setLayout(headerLayout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      new Label(header, SWT.NONE).setText(i18n.tr("Connection state:"));
      connectionStateLabel = new Label(header, SWT.NONE);
      connectionStateLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      new Label(header, SWT.NONE).setText(i18n.tr("Backend:"));
      backendLabel = new Label(header, SWT.NONE);
      backendLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      new Label(header, SWT.NONE).setText(i18n.tr("Capabilities:"));
      capabilitiesLabel = new Label(header, SWT.NONE);
      capabilitiesLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      new Label(header, SWT.NONE).setText(i18n.tr("Last discovery:"));
      lastDiscoveryLabel = new Label(header, SWT.NONE);
      lastDiscoveryLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      new Label(header, SWT.NONE).setText(i18n.tr("Host alias sync:"));
      syncStatusLabel = new Label(header, SWT.NONE);
      syncStatusLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      final String[] names = {
            i18n.tr("ID"),
            i18n.tr("Name"),
            i18n.tr("External ID"),
            i18n.tr("Type"),
            i18n.tr("State"),
            i18n.tr("Provider state"),
            i18n.tr("Sampling"),
            i18n.tr("In scope"),
            i18n.tr("Last discovery"),
            i18n.tr("Status")
      };
      final int[] widths = { 60, 180, 120, 80, 80, 100, 80, 70, 140, 80 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "ObservationPointsTable");
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.setLabelProvider(new ObservationPointListLabelProvider());
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new ObservationPointListComparator());
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);

      ObservationPointFilter filter = new ObservationPointFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createActions();
      createContextMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object instanceof ObservationPoint) && object.isDirectChildOf(getObjectId()))
                  getDisplay().asyncExec(() -> refresh());
            }
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(this, viewer, true);

      actionIncludeInScope = new Action(i18n.tr("&Include in scope")) {
         @Override
         public void run()
         {
            setScopeForSelection(true);
         }
      };

      actionExcludeFromScope = new Action(i18n.tr("E&xclude from scope")) {
         @Override
         public void run()
         {
            setScopeForSelection(false);
         }
      };
   }

   /**
    * Set in-scope flag for selected observation points.
    *
    * @param inScope new in-scope flag
    */
   private void setScopeForSelection(boolean inScope)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      final List<Long> objects = new ArrayList<>();
      for(Object o : selection.toList())
      {
         if (o instanceof ObservationPoint)
            objects.add(((ObservationPoint)o).getObjectId());
      }
      if (objects.isEmpty())
         return;

      new Job(i18n.tr("Updating observation point scope"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Long id : objects)
            {
               NXCObjectModificationData md = new NXCObjectModificationData(id);
               md.setInScope(inScope);
               session.modifyObject(md);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update observation point scope");
         }
      }.start();
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, viewer, null) {
         @Override
         protected void fillContextMenu()
         {
            TrafficObserverView.this.fillContextMenu(this);
            add(new Separator());
            super.fillContextMenu();
         }
      };

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionIncludeInScope);
      manager.add(actionExcludeFromScope);
      manager.add(new Separator());
      manager.add(actionExportToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportToCsv);
   }

   /**
    * Get display text for observer connection state.
    *
    * @param state connection state code
    * @return display text
    */
   private String getConnectionStateText(int state)
   {
      switch(state)
      {
         case TrafficObserver.STATE_CONNECTED:
            return i18n.tr("Connected");
         case TrafficObserver.STATE_UNREACHABLE:
            return i18n.tr("Unreachable");
         case TrafficObserver.STATE_AUTH_FAILURE:
            return i18n.tr("Authentication failure");
         default:
            return i18n.tr("Unknown");
      }
   }

   /**
    * Build readable capability list.
    *
    * @param observer traffic observer
    * @return capability list text
    */
   private String getCapabilitiesText(TrafficObserver observer)
   {
      List<String> caps = new ArrayList<>();
      if (observer.hasCapability(TrafficObserver.CAP_POINT_L7))
         caps.add(i18n.tr("point L7"));
      if (observer.hasCapability(TrafficObserver.CAP_POINT_TOP_TALKERS))
         caps.add(i18n.tr("top talkers"));
      if (observer.hasCapability(TrafficObserver.CAP_POINT_DSCP))
         caps.add(i18n.tr("DSCP"));
      if (observer.hasCapability(TrafficObserver.CAP_HOST_L7))
         caps.add(i18n.tr("host L7"));
      if (observer.hasCapability(TrafficObserver.CAP_HOST_PEERS))
         caps.add(i18n.tr("host peers"));
      if (observer.hasCapability(TrafficObserver.CAP_HOST_SET_AUTHORITATIVE))
         caps.add(i18n.tr("authoritative host set"));
      if (observer.hasCapability(TrafficObserver.CAP_HISTORICAL_TIMESERIES))
         caps.add(i18n.tr("historical timeseries"));
      if (observer.hasCapability(TrafficObserver.CAP_SYNC_HOST_ALIASES))
         caps.add(i18n.tr("host alias sync"));
      return String.join(", ", caps);
   }

   /**
    * Check if host alias sync is enabled in observer's sync configuration.
    *
    * @param observer traffic observer
    * @return true if host alias sync is enabled
    */
   private static boolean isHostAliasSyncEnabled(TrafficObserver observer)
   {
      String syncConfig = observer.getSyncConfig();
      if ((syncConfig == null) || syncConfig.isEmpty())
         return false;
      try
      {
         JsonObject hostAliases = JsonParser.parseString(syncConfig).getAsJsonObject().getAsJsonObject("hostAliases");
         return (hostAliases != null) && hostAliases.has("enabled") && hostAliases.get("enabled").getAsBoolean();
      }
      catch(Exception e)
      {
         return false;
      }
   }

   /**
    * Update host alias sync status line (asynchronously reads sync status metrics when sync is active)
    */
   private void updateSyncStatus()
   {
      TrafficObserver observer = (TrafficObserver)getObject();
      if (observer == null)
      {
         syncStatusLabel.setText("");
         return;
      }

      if (!observer.hasCapability(TrafficObserver.CAP_SYNC_HOST_ALIASES))
      {
         syncStatusLabel.setText(i18n.tr("not supported by backend"));
         return;
      }

      if (!isHostAliasSyncEnabled(observer))
      {
         syncStatusLabel.setText(i18n.tr("disabled"));
         return;
      }

      syncStatusLabel.setText(i18n.tr("enabled"));
      final long objectId = observer.getObjectId();
      new Job(i18n.tr("Reading host alias sync status"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long lastRun;
            String records, errors;
            try
            {
               lastRun = Long.parseLong(session.queryMetric(objectId, DataOrigin.TRAFFIC_OBSERVER, "Sync.HostAliases.LastRun").trim());
               records = session.queryMetric(objectId, DataOrigin.TRAFFIC_OBSERVER, "Sync.HostAliases.RecordsSynced").trim();
               errors = session.queryMetric(objectId, DataOrigin.TRAFFIC_OBSERVER, "Sync.HostAliases.Errors").trim();
            }
            catch(Exception e)
            {
               return;  // leave static "enabled" text (server may not serve sync status metrics)
            }

            final String text = (lastRun > 0)
                  ? String.format(i18n.tr("enabled; last run %s, %s records, %s failed runs"),
                        DateFormatFactory.getDateTimeFormat().format(new Date(lastRun * 1000)), records, errors)
                  : i18n.tr("enabled; not run yet");
            runInUIThread(() -> {
               if (!syncStatusLabel.isDisposed() && (getObjectId() == objectId))
               {
                  syncStatusLabel.setText(text);
                  syncStatusLabel.getParent().layout(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read host alias sync status");
         }
      }.start();
   }

   /**
    * Update header from current object
    */
   private void updateHeader()
   {
      TrafficObserver observer = (TrafficObserver)getObject();
      if (observer == null)
      {
         connectionStateLabel.setText("");
         backendLabel.setText("");
         capabilitiesLabel.setText("");
         lastDiscoveryLabel.setText("");
         syncStatusLabel.setText("");
         return;
      }

      connectionStateLabel.setText(getConnectionStateText(observer.getConnectionState()));

      String product = observer.getBackendProduct();
      if (!product.isEmpty())
         backendLabel.setText(String.format("%s %s (%s)", product, observer.getBackendVersion(), observer.getBackendEdition()));
      else
         backendLabel.setText(i18n.tr("Unknown"));

      capabilitiesLabel.setText(getCapabilitiesText(observer));

      StringBuilder discovery = new StringBuilder();
      if ((observer.getLastDiscoveryTime() != null) && (observer.getLastDiscoveryTime().getTime() > 0))
      {
         discovery.append(DateFormatFactory.getDateTimeFormat().format(observer.getLastDiscoveryTime()));
         discovery.append(" - ");
         discovery.append((observer.getLastDiscoveryStatus() == 0) ? i18n.tr("successful") : i18n.tr("failed"));
         if (!observer.getLastDiscoveryMessage().isEmpty())
         {
            discovery.append(" (");
            discovery.append(observer.getLastDiscoveryMessage());
            discovery.append(')');
         }
      }
      else
      {
         discovery.append(i18n.tr("never"));
      }
      lastDiscoveryLabel.setText(discovery.toString());

      updateSyncStatus();

      connectionStateLabel.getParent().layout(true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      updateHeader();
      if (getObject() != null)
         viewer.setInput(getObject().getAllChildren(AbstractObject.OBJECT_OBSERVATIONPOINT).toArray());
      else
         viewer.setInput(new Object[0]);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if ((object != null) && isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      refresh();
      super.activate();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }
}
