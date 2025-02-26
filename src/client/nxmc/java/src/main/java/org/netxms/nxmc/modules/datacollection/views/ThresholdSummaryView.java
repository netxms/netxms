/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.ThresholdStateChange;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.ShowHistoricalDataMenuItems;
import org.netxms.nxmc.modules.datacollection.views.helpers.ThresholdTreeComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.ThresholdTreeContentProvider;
import org.netxms.nxmc.modules.datacollection.views.helpers.ThresholdTreeFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.ThresholdTreeLabelProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Threshold violation summary page
 */
public class ThresholdSummaryView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ThresholdSummaryView.class);
   
   public static final int COLUMN_NODE = 0;
   public static final int COLUMN_STATUS = 1;
   public static final int COLUMN_PARAMETER = 2;
   public static final int COLUMN_VALUE = 3;
   public static final int COLUMN_CONDITION = 4;
   public static final int COLUMN_EVENT = 5;
   public static final int COLUMN_TIMESTAMP = 6;

   private SortableTreeViewer viewer;
   private SessionListener sessionListener;
   private boolean subscribed = false;
   private boolean refreshScheduled = false;

   /**
    * Create web service manager view
    */
   public ThresholdSummaryView()
   {
      super(LocalizationHelper.getI18n(ThresholdSummaryView.class).tr("Thresholds"), ResourceManager.getImageDescriptor("icons/object-views/thresholds.png"), "objects.threshold-summary", true);

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.THRESHOLD_STATE_CHANGED)
            {
               final ThresholdStateChange stateChange = (ThresholdStateChange)n.getObject();
               getDisplay().asyncExec(() -> processNotification(stateChange));
            }
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 35;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof EntireNetwork) || (context instanceof Zone) || (context instanceof Subnet) || (context instanceof ServiceRoot) || (context instanceof Container) ||
            (context instanceof Rack) || (context instanceof Collector) || (context instanceof Cluster);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("Node"), i18n.tr("Status"), i18n.tr("Metric"), i18n.tr("Value"), i18n.tr("Condition"), i18n.tr("Event"), i18n.tr("Since") };
      final int[] widths = { 200, 100, 250, 100, 100, 250, 140 };
      viewer = new SortableTreeViewer(parent, names, widths, COLUMN_NODE, SWT.UP, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ThresholdTreeContentProvider());
      viewer.setLabelProvider(new ThresholdTreeLabelProvider());
      viewer.setComparator(new ThresholdTreeComparator());

      ThresholdTreeFilter filter = new ThresholdTreeFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createContextMenu();

      session.addListener(sessionListener);
   }

   /**
    * Process threshold state change
    * 
    * @param stateChange state change information
    */
   private void processNotification(ThresholdStateChange stateChange)
   {
      AbstractObject object = getObject();
      if (refreshScheduled || (object == null) || ((object.getObjectId() != stateChange.getObjectId()) && !object.isParentOf(stateChange.getObjectId())))
         return;

      refreshScheduled = true;
      getDisplay().timerExec(500, () -> {
         refreshScheduled = false;
         refresh();
      });
   }

   /**
    * Create pop-up menu for alarm list
    */
   private void createContextMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Check if selection is DCI, tbale or mixed
    */
   public int getDciSelectionType()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return DataCollectionObject.DCO_TYPE_GENERIC;

      boolean isDci = false;
      boolean isTable = false;
      for(Object dcObject : selection.toList())
      {
         if ((dcObject instanceof DataCollectionTable) || ((dcObject instanceof DciValue) && ((DciValue)dcObject).getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE))
         {
            isTable = true;
         }
         else
         {
            isDci = true;
         }
      }
      return (isTable & isDci) ? DataCollectionObject.DCO_TYPE_GENERIC : isTable ? DataCollectionObject.DCO_TYPE_TABLE : DataCollectionObject.DCO_TYPE_ITEM;
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(IMenuManager mgr)
   {
      ShowHistoricalDataMenuItems.populateMenu(mgr, this, getObject(), viewer, getDciSelectionType());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
	}   

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
	@Override
	public void refresh()
	{
      if (!isActive())
         return;

      final long rootId = getObjectId();
      if (rootId == 0)
      {
         viewer.setInput(new Object[0]);
         return;
      }

      new Job(i18n.tr("Reading threshold summary"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ThresholdViolationSummary> data = session.getThresholdSummary(rootId);
            runInUIThread(() -> {
               if (isClientAreaDisposed() || (rootId != getObjectId()))
                  return;
               viewer.setInput(data);
               viewer.expandAll();
            });
            if (!subscribed)
            {
               session.subscribe(NXCSession.CHANNEL_DC_THRESHOLDS);
               subscribed = true;
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read threshold summary");
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);

      if (subscribed)
      {
         Job job = new Job(i18n.tr("Unsubscribing from threshold notifications"), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.unsubscribe(NXCSession.CHANNEL_DC_THRESHOLDS);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot change event subscription");
            }
         };
         job.setUser(false);
         job.setSystem(true);
         job.start();
      }

      super.dispose();
   }
}
