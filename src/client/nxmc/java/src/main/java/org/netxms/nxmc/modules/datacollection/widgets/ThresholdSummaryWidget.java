/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.ShowHistoricalDataMenuItems;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.ThresholdTreeComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.ThresholdTreeContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.ThresholdTreeLabelProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.xnap.commons.i18n.I18n;

/**
 * Widget to show threshold violation summary
 */
public class ThresholdSummaryWidget extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(ThresholdSummaryWidget.class);
   
	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_PARAMETER = 2;
	public static final int COLUMN_VALUE = 3;
	public static final int COLUMN_CONDITION = 4;
	public static final int COLUMN_TIMESTAMP = 5;
	
	private AbstractObject object;
	private ObjectView viewPart;
	private SortableTreeViewer viewer;
	private VisibilityValidator visibilityValidator;
	private boolean subscribed = false;
	private boolean refreshScheduled = false;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ThresholdSummaryWidget(Composite parent, int style, ObjectView viewPart, VisibilityValidator visibilityValidator)
	{
		super(parent, style);
		this.viewPart = viewPart;
		this.visibilityValidator = visibilityValidator;

		setLayout(new FillLayout());

		final String[] names = { i18n.tr("Node"),  i18n.tr("Status"), i18n.tr("Parameter"), i18n.tr("Value"), i18n.tr("Condition"), i18n.tr("Since") };
		final int[] widths = { 200, 100, 250, 100, 100, 140 };
		viewer = new SortableTreeViewer(this, names, widths, COLUMN_NODE, SWT.UP, SWT.FULL_SELECTION);
		viewer.setContentProvider(new ThresholdTreeContentProvider());
		viewer.setLabelProvider(new ThresholdTreeLabelProvider());		
		viewer.setComparator(new ThresholdTreeComparator());
		
		createPopupMenu();
		
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (!subscribed)
               return;
            
            final NXCSession session = Registry.getSession();
            Job job = new Job("Unsubscribe from threshold notifications", null) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  session.unsubscribe(NXCSession.CHANNEL_DC_THRESHOLDS);
               }
               
               @Override
               protected String getErrorMessage()
               {
                  return "Cannot change event subscription";
               }
            };
            job.setUser(false);
            job.setSystem(true);
            job.start();
         }
      });
		
		Registry.getSession().addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.THRESHOLD_STATE_CHANGED)
            {
               final ThresholdStateChange stateChange = (ThresholdStateChange)n.getObject();
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     processNotification(stateChange);
                  }
               });
            }
         }
      });
	}
	
	/**
	 * Process threshold state change
	 * 
	 * @param stateChange state change information
	 */
	private void processNotification(ThresholdStateChange stateChange)
	{
	   if (refreshScheduled || (object == null) ||
	       ((object.getObjectId() != stateChange.getObjectId()) && !object.isParentOf(stateChange.getObjectId())))
	      return;

	   refreshScheduled = true;
	   getDisplay().timerExec(500, new Runnable() {
         @Override
         public void run()
         {
            refreshScheduled = false;
            refresh();
         }
      });
	}

   /**
    * Create pop-up menu for alarm list
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

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
         if ((dcObject instanceof DataCollectionTable) || ((dcObject instanceof DciValue) &&
            ((DciValue)dcObject).getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE))
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
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {      
      ShowHistoricalDataMenuItems.populateMenu(mgr, viewPart, object, viewer, getDciSelectionType());
   }

	/**
	 * Refresh widget
	 */
	public void refresh()
	{
	   if (visibilityValidator != null && !visibilityValidator.isVisible())
	      return;
	   
		if (object == null)
		{
			viewer.setInput(new ArrayList<ThresholdViolationSummary>(0));
			return;
		}
		
		final NXCSession session = Registry.getSession();
		final long rootId = object.getObjectId();
		Job job = new Job(i18n.tr("Get threshold summary"), viewPart) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<ThresholdViolationSummary> data = session.getThresholdSummary(rootId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (isDisposed() || (object == null) || (rootId != object.getObjectId()))
							return;
						viewer.setInput(data);
						viewer.expandAll();
					}
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
				return i18n.tr("Cannot get threshold summary");
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param object the object to set
	 */
	public void setObject(AbstractObject object)
	{
		this.object = object;
		refresh();
	}
}
