/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.HistoricalDataSelectionDialog;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Historical data view
 */
public class HistoricalDataView extends ViewWithContext
{
   private final I18n i18n = LocalizationHelper.getI18n(HistoricalDataView.class);

	// Columns
   public static final int COLUMN_TIMESTAMP = 0;
   public static final int COLUMN_VALUE = 1;
   public static final int COLUMN_FORMATTED_VALUE = 2;
   public static final int COLUMN_RAW_VALUE = 3;

   private NXCSession session;
	private long contextId;
   private long ownerId;
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
   private Composite infoArea;
   private Label labelDciId;
   private Label labelDciName;
   private RoundedLabel labelDciDescription;
   private Label labelDciUnits;
   private Label labelTableInfo;

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

   /**
    * Create new historical data view.
    *
    * @param contextObject context object
    * @param ownerId DCI owner ID
    * @param dciId DCI ID
    */
   public HistoricalDataView(AbstractObject contextObject, long ownerId, long dciId)
   {
      this(contextObject, ownerId, dciId, null, null, null);
   }

   /**
    * Create new historical data view.
    *
    * @param contextObject context object
    * @param ownerId DCI owner ID
    * @param dciId DCI ID
    * @param tableName table name
    * @param instance instance for table data
    * @param column column name for table data
    */
   public HistoricalDataView(AbstractObject contextObject, long ownerId, long dciId, String tableName, String instance, String column)
   {
      super(LocalizationHelper.getI18n(HistoricalDataView.class).tr("Historical Data"), ResourceManager.getImageDescriptor("icons/object-views/history-view.png"), 
            buildId(contextObject, dciId, tableName, instance, column), true);

      session = Registry.getSession();

      contextId = contextObject.getObjectId();
      this.ownerId = ownerId;
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
      view.contextId = contextId;
      view.dciId = dciId;
      view.ownerId = ownerId;
      view.tableName = tableName;
      view.instance = instance;
      view.column = column;
      view.fullName = fullName;
      view.nodeName = nodeName;
      view.timeFrom = timeFrom;
      view.timeTo = timeTo;
      view.recordLimit = recordLimit;
      return view;
   }

   /**
    * Default constructor for use by cloneView()
    */
   public HistoricalDataView()
   {
      super(LocalizationHelper.getI18n(HistoricalDataView.class).tr("Historical Data"), ResourceManager.getImageDescriptor("icons/object-views/history-view.png"), UUID.randomUUID().toString(), true);

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

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      infoArea = new Composite(parent, SWT.NONE);
      infoArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 9;
      infoArea.setLayout(layout);

      labelDciId = new Label(infoArea, SWT.NONE);
      labelDciId.setText(Long.toString(dciId));
      createSeparator(infoArea);
      labelDciDescription = new RoundedLabel(infoArea);
      createSeparator(infoArea);
      labelDciName = new Label(infoArea, SWT.NONE);
      createSeparator(infoArea);
      labelDciUnits = new Label(infoArea, SWT.NONE);
      createSeparator(infoArea);
      labelTableInfo = new Label(infoArea, SWT.NONE);

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

		final String[] names = (tableName != null) ? 
         new String[] { i18n.tr("Timestamp"), i18n.tr("Value") } :
         new String[] { i18n.tr("Timestamp"), i18n.tr("Value"), i18n.tr("Formatted value"), i18n.tr("Raw value") };
      final int[] widths = { 180, 400, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new HistoricalDataLabelProvider());
		viewer.setComparator(new HistoricalDataComparator());
		HistoricalDataFilter filter = new HistoricalDataFilter();
      viewer.addFilter(filter);
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		setFilterClient(viewer, filter);

		createActions();
		createContextMenu();

		refresh();
	}

   /**
    * Create vertical separator.
    *
    * @param parent parent composite
    */
   private static void createSeparator(Composite parent)
   {
      GridData gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 28;
      new Label(parent, SWT.SEPARATOR | SWT.VERTICAL).setLayoutData(gd);
   }

	/**
	 * Create actions
	 */
	private void createActions()
	{		
      actionSelectRange = new Action(i18n.tr("Select data &range..."), SharedIcons.DATES) {
			@Override
			public void run()
			{
				selectRange();
			}
		};
      addKeyBinding("M1+R", actionSelectRange);

      actionDeleteDciEntry = new Action(i18n.tr("&Delete value"), SharedIcons.DELETE_OBJECT) {
		   @Override
		   public void run ()
		   {
		      deleteValue();
		   }
		};
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSelectRange);
      manager.add(new Separator());
      manager.add(actionExportAllToCsv);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionSelectRange);
      manager.add(new Separator());
      manager.add(actionExportAllToCsv);
      super.fillLocalMenu(manager);
   }

   /**
    * Create pop-up menu
    */
	private void createContextMenu()
	{
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));
      viewer.getControl().setMenu(manager.createContextMenu(viewer.getControl()));
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionSelectRange);
      manager.add(new Separator());
		manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);		
      manager.add(new Separator());
      manager.add(actionDeleteDciEntry);
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

      new Job(i18n.tr("Reading DCI data from server"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{            
			   final DataSeries data;
			   if (tableName != null)
			      data = session.getCollectedTableData(ownerId, dciId, instance, column, timeFrom, timeTo, recordLimit);
			   else
               data = session.getCollectedData(ownerId, dciId, timeFrom, timeTo, recordLimit, HistoricalDataType.RAW_AND_PROCESSED);

            runInUIThread(() -> {
               labelDciName.setText(data.getDciName());
               labelDciDescription.setText(data.getDciDescription());
               labelDciDescription.setLabelBackground(StatusDisplayInfo.getStatusBackgroundColor(data.getActiveThresholdSeverity()));
               if (data.getMeasurementUnit() != null)
               {
                  labelDciUnits.setText(data.getMeasurementUnit().getName());
               }
               else
               {
                  labelDciUnits.setText(i18n.tr("Measurement units not set"));
               }
               if (tableName != null)
               {
                  labelTableInfo.setText(tableName);
               }

               ((HistoricalDataLabelProvider)viewer.getLabelProvider()).setDataFormatter(data.getDataFormatter());
               viewer.setInput(data.getValues());
               updateInProgress = false;
               infoArea.layout();
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

   /**
    * Delete value from collected data
    */
	private void deleteValue()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
	      return;

      new Job("Delete DCI value", null) {
         @SuppressWarnings("unchecked")
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<DciDataRow> list = selection.toList();
            for(DciDataRow r : list)
               session.deleteDciEntry(contextId, dciId, r.getTimestamp().getTime() / 1000); // Convert back to seconds

            final DataSeries data;
            if (tableName != null)
               data = session.getCollectedTableData(contextId, dciId, instance, column, timeFrom, timeTo, recordLimit);
            else
               data = session.getCollectedData(contextId, dciId, timeFrom, timeTo, recordLimit, HistoricalDataType.RAW_AND_PROCESSED);

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
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == contextId);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if ((newContext == null) || !(newContext instanceof AbstractObject) || (((AbstractObject)newContext).getObjectId() != contextId))
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

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("contextId", contextId);
      memento.set("ownerId", ownerId);
      memento.set("dciId", dciId);
      if (tableName != null)
         memento.set("tableName", tableName);
      if (instance != null)
         memento.set("instance", instance);
      if (column != null)
         memento.set("column", column);
      if (timeFrom != null)
         memento.set("timeFrom", timeFrom.getTime());         
      if (timeTo != null)
         memento.set("timeTo", timeTo.getTime());       
      
      memento.set("recordLimit", recordLimit);     
   }  


   /**
    * Restore view state
    * 
    * @param memento memento to restore the state
    * @throws ViewNotRestoredException 
    */
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      ownerId = memento.getAsLong("ownerId", 0);
      contextId = memento.getAsLong("contextId", 0);
      dciId = memento.getAsLong("dciId", 0);
      tableName = memento.getAsString("tableName", null); 
      instance = memento.getAsString("instance", null);
      column = memento.getAsString("column", null);
      
      AbstractObject object = session.findObjectById(contextId);
      if (object != null)
         nodeName = object.getObjectName();
      String dciName =  (tableName == null ? Long.toString(dciId) : tableName);
      fullName = nodeName + ": [" + dciName + "]";
      setName(dciName);
      
      long timestmap = memento.getAsLong("timeFrom", 0);
      if (timestmap != 0)
         timeFrom = new Date(timestmap);
      timestmap = memento.getAsLong("timeTo", 0);
      if (timestmap != 0)
         timeTo = new Date(timestmap);

      recordLimit = memento.getAsInteger("recordLimit", recordLimit);
      
      super.restoreState(memento);
   }

   @Override
   protected Object restoreContext(Memento memento)
   {
      return session.findObjectById(contextId);
   }
}
