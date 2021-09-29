/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.HistoricalDataSelectionDialog;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Historical data view
 */
public class HistoricalDataView extends ViewWithContext
{
   private static final I18n i18n = LocalizationHelper.getI18n(HistoricalDataView.class);
	
	// Columns
   public static final int COLUMN_TIME = 0;
   public static final int COLUMN_DATA = 1;
	
	private NXCSession session;
	private long objectId;
	private long dciId;
   private String fullName;
	private String nodeName;
	private String tableName;
	private String instance;
	private String column;
	private SortableTableViewer viewer;
	private Date timeFrom = null;
	private Date timeTo = null;
	private int recordLimit = 4096;
	private boolean updateInProgress = false;
	private Action actionSelectRange;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	private Action actionDeleteDciEntry;

   /**
    * Build view ID
    *
    * @param object context object
    * @param items list of DCIs to show
    * @return view ID
    */
   private static String buildId(AbstractObject object, long dciId, String tableName, String instance, String column)
   {
      StringBuilder sb = new StringBuilder("HistoricalGraphView");
      if (object != null)
      {
         sb.append('#');
         sb.append(object.getObjectId());
      }
      sb.append('#');
      sb.append(dciId);
      if (tableName != null)
      {
         sb.append('#');
         sb.append(tableName);
      }
      if (instance != null)
      {
         sb.append('#');
         sb.append(instance);
      }
      if (column != null)
      {
         sb.append('#');
         sb.append(column);
      }
      
      return sb.toString();
   }	

   public HistoricalDataView(AbstractObject contextObject, long dciId, String tableName, String instance, String column)
   {
      super(i18n.tr("Historical Data"), ResourceManager.getImageDescriptor("icons/object-views/data_history.gif"), 
            buildId(contextObject, dciId, tableName, instance, column), true);

      session = Registry.getSession();
      
      objectId = contextObject.getObjectId();
      this.dciId = dciId;
      this.tableName = tableName; 
      this.instance = instance; 
      this.column = column;
      
      nodeName = contextObject.getObjectName();
      String dciName =  (tableName == null ? Long.toString(dciId) : tableName);
      fullName = nodeName + ": [" + dciName + "]";
      setName(dciName);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      HistoricalDataView view = (HistoricalDataView)super.cloneView();
      view.objectId = objectId;
      view.dciId = dciId;
      view.tableName = tableName;
      view.instance = instance;
      view.column = column;
      view.fullName = fullName;
      view.nodeName = nodeName;
      return view;
   }

   /**
    * Default constructor for use by cloneView()
    */
   public HistoricalDataView()
   {
      super(i18n.tr("Historical Data"), ResourceManager.getImageDescriptor("icons/object-views/data_history.gif"), UUID.randomUUID().toString(), true);

      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return fullName;
   }

   @Override
   protected void createContent(Composite parent)
   {
		final String[] names = (tableName != null) ? 
         new String[] { i18n.tr("Timestamp"), i18n.tr("Value") } :
         new String[] { i18n.tr("Timestamp"), i18n.tr("Value"), "Raw value" };
		final int[] widths = { 150, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new HistoricalDataLabelProvider());
		viewer.setComparator(new HistoricalDataComparator());
		HistoricalDataFilter filter = new HistoricalDataFilter();
		viewer.setFilters(filter);
		setFilterClient(viewer, filter);

		createActions();
		createPopupMenu();
		
		refresh();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{		
		actionSelectRange = new Action(i18n.tr("Select data &range...")) {
			@Override
			public void run()
			{
				selectRange();
			}
		};
		
		actionDeleteDciEntry = new Action("Delete entry") {
		   @Override
		   public void run ()
		   {
		      deleteDciEntry();
		   }
		};
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}
	
	/**
	 * Create pop-up menu
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
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
      manager.add(actionDeleteDciEntry);
		manager.add(actionSelectRange);
		manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);		
	}

	/**
	 * Refresh data
	 */
	@Override
   public void refresh()
	{
		if (updateInProgress)
			return;
		updateInProgress = true;
		
		new Job(i18n.tr("Read DCI data from server"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
			   final DciData data;
			   if (tableName != null)
			      data = session.getCollectedTableData(objectId, dciId, instance, column, timeFrom, timeTo, recordLimit);
			   else
			      data = session.getCollectedData(objectId, dciId, timeFrom, timeTo, recordLimit, HistoricalDataType.BOTH);
			   
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(data.getValues());
						updateInProgress = false;
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
			   return String.format(i18n.tr("Cannot get data for DCI %s:[%d]"), nodeName, dciId);
			}
		}.start();
	}
	
	/**
	 * Select data range for display
	 */
	private void selectRange()
	{
		HistoricalDataSelectionDialog dlg = new HistoricalDataSelectionDialog(getWindow().getShell(), recordLimit, timeFrom, timeTo);
		if (dlg.open() == Window.OK)
		{
			recordLimit = dlg.getMaxRecords();
			timeFrom = dlg.getTimeFrom();
			timeTo = dlg.getTimeTo();
			refresh();
		}
	}
	
	private void deleteDciEntry()
	{
	   final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
	   if (selection.size() == 0)
	      return;
	   
	   new Job("Delete DCI entry", null) {
         @SuppressWarnings("unchecked")
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<DciDataRow> list = selection.toList();
            for(DciDataRow r : list)
               session.deleteDciEntry(objectId, dciId, r.getTimestamp().getTime() / 1000); // Convert back to seconds
            
            final DciData data;
            if (tableName != null)
               data = session.getCollectedTableData(objectId, dciId, instance, column, timeFrom, timeTo, recordLimit);
            else
               data = session.getCollectedData(objectId, dciId, timeFrom, timeTo, recordLimit, HistoricalDataType.BOTH);
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(data.getValues());
                  updateInProgress = false;
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete DCI entry";
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == objectId);
   }

   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if ((newContext == null) || !(newContext instanceof AbstractObject) || (((AbstractObject)newContext).getObjectId() != objectId))
         return;
      
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
