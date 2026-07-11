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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.traffic.widgets.TrafficQueryTable;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Traffic" tab for nodes observed by traffic observation points: observation point
 * records for the node with per-host live statistics, application breakdown, and peers.
 */
public class NodeTrafficView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(NodeTrafficView.class);

   private static final String[] STAT_METRICS = { "BytesIn", "BytesOut", "PacketsIn", "PacketsOut", "ActiveFlows", "TcpRetransmits" };

   private TrafficQueryTable hostRecordsTable;
   private Label[] statLabels = new Label[STAT_METRICS.length];
   private TrafficQueryTable l7Table;
   private TrafficQueryTable peersTable;
   private volatile String selectedInstance = null;

   /**
    * Create view
    */
   public NodeTrafficView()
   {
      super(LocalizationHelper.getI18n(NodeTrafficView.class).tr("Traffic"),
            ResourceManager.getImageDescriptor("icons/object-views/chart-bar.png"), "objects.node-traffic", false);
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
      return 70;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof AbstractNode) && ((AbstractNode)context).hasObservationPoints();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());

      SashForm content = new SashForm(parent, SWT.VERTICAL);

      Group recordsGroup = new Group(content, SWT.NONE);
      recordsGroup.setText(i18n.tr("Observation points"));
      recordsGroup.setLayout(new FillLayout());
      hostRecordsTable = new TrafficQueryTable(recordsGroup, SWT.NONE, this, "NodeTrafficRecords", i18n.tr("Reading observation point records"),
            () -> (getObject() != null) ? session.queryTable(getObjectId(), DataOrigin.INTERNAL, "Traffic.ObservationPoints") : null);
      hostRecordsTable.getViewer().addSelectionChangedListener((event) -> {
         IStructuredSelection selection = (IStructuredSelection)event.getSelection();
         String instance = null;
         if (selection.getFirstElement() instanceof TableRow)
         {
            Table data = hostRecordsTable.getCurrentData();
            int column = (data != null) ? data.getColumnIndex("INSTANCE") : -1;
            if (column != -1)
               instance = ((TableRow)selection.getFirstElement()).getValue(column);
         }
         if ((instance != null) ? !instance.equals(selectedInstance) : (selectedInstance != null))
         {
            selectedInstance = instance;
            refreshHostDetails();
         }
      });

      Composite details = new Composite(content, SWT.NONE);
      GridLayout detailsLayout = new GridLayout();
      detailsLayout.marginWidth = 0;
      detailsLayout.marginHeight = 0;
      details.setLayout(detailsLayout);

      final String[] statTitles = {
            i18n.tr("Bytes (in)"),
            i18n.tr("Bytes (out)"),
            i18n.tr("Packets (in)"),
            i18n.tr("Packets (out)"),
            i18n.tr("Active flows"),
            i18n.tr("TCP retransmits")
      };

      Composite statHeader = new Composite(details, SWT.NONE);
      statHeader.setLayout(new GridLayout(STAT_METRICS.length, true));
      statHeader.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      for(int i = 0; i < STAT_METRICS.length; i++)
      {
         Composite cell = new Composite(statHeader, SWT.NONE);
         cell.setLayout(new GridLayout());
         cell.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

         Label title = new Label(cell, SWT.CENTER);
         title.setText(statTitles[i]);
         title.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));

         statLabels[i] = new Label(cell, SWT.CENTER);
         statLabels[i].setText("-");
         statLabels[i].setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));
      }

      SashForm tables = new SashForm(details, SWT.HORIZONTAL);
      tables.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Group l7Group = new Group(tables, SWT.NONE);
      l7Group.setText(i18n.tr("Application breakdown"));
      l7Group.setLayout(new FillLayout());
      l7Table = new TrafficQueryTable(l7Group, SWT.NONE, this, "NodeTrafficL7", i18n.tr("Reading host application breakdown"),
            () -> queryHostTable("L7Breakdown", TrafficObserver.CAP_HOST_L7));
      l7Table.setNoDataMessage(i18n.tr("Select observation point record"));
      l7Table.setSortColumn("BYTES_RCVD", SWT.DOWN);

      Group peersGroup = new Group(tables, SWT.NONE);
      peersGroup.setText(i18n.tr("Peers"));
      peersGroup.setLayout(new FillLayout());
      peersTable = new TrafficQueryTable(peersGroup, SWT.NONE, this, "NodeTrafficPeers", i18n.tr("Reading host peers"),
            () -> queryHostTable("Peers", TrafficObserver.CAP_HOST_PEERS));
      peersTable.setNoDataMessage(i18n.tr("Select observation point record"));

      content.setWeights(new int[] { 30, 70 });
   }

   /**
    * Query host-level table for currently selected observation point record.
    *
    * @param name host table base name
    * @return table or null if nothing is selected
    * @throws Exception on query failure
    */
   private Table queryHostTable(String name, long requiredCapability) throws Exception
   {
      String instance = selectedInstance;
      if ((instance == null) || (getObject() == null))
         return null;

      // Skip query if the backend serving the selected point lacks the capability
      int separator = instance.indexOf(':');
      if (separator > 0)
      {
         try
         {
            AbstractObject point = session.findObjectById(Long.parseLong(instance.substring(0, separator)));
            if (point instanceof ObservationPoint)
            {
               AbstractObject observer = session.findObjectById(((ObservationPoint)point).getObserverId());
               if ((observer instanceof TrafficObserver) && !((TrafficObserver)observer).hasCapability(requiredCapability))
                  return null;
            }
         }
         catch(NumberFormatException e)
         {
         }
      }

      return session.queryTable(getObjectId(), DataOrigin.TRAFFIC_OBSERVER, "Host." + name + "(" + instance + ")");
   }

   /**
    * Refresh per-host statistics and tables for the selected record
    */
   private void refreshHostDetails()
   {
      final String instance = selectedInstance;
      final long objectId = getObjectId();

      l7Table.refresh(null);
      peersTable.refresh(null);

      if ((instance == null) || (objectId == 0))
      {
         for(Label l : statLabels)
            l.setText("-");
         return;
      }

      new Job(i18n.tr("Reading host traffic statistics"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String[] values = new String[STAT_METRICS.length];
            for(int i = 0; i < STAT_METRICS.length; i++)
            {
               try
               {
                  values[i] = session.queryMetric(objectId, DataOrigin.TRAFFIC_OBSERVER, "Host." + STAT_METRICS[i] + "(" + instance + ")");
               }
               catch(Exception e)
               {
                  values[i] = null;
               }
            }
            runInUIThread(() -> {
               if (statLabels[0].isDisposed() || !instance.equals(selectedInstance))
                  return;
               for(int i = 0; i < STAT_METRICS.length; i++)
                  statLabels[i].setText((values[i] != null) ? values[i] : "-");
               statLabels[0].getParent().getParent().layout(true, true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read host traffic statistics");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      hostRecordsTable.refresh(null);
      refreshHostDetails();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      selectedInstance = null;
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
}
