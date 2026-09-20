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
package org.netxms.nxmc.modules.traffic.widgets;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableItemComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableLabelProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * Table viewer for live traffic data queries. Columns are built dynamically from
 * the query result; the query itself is supplied by the owning view.
 */
public class TrafficQueryTable extends Composite
{
   /**
    * Data query executed on refresh (in a background job). May return null to
    * indicate that no data is available (the widget then shows noDataMessage).
    */
   public interface TableQuery
   {
      public Table read() throws Exception;
   }

   /**
    * Resolver for NetXMS object related to table row. When set, double click on a row
    * and "Go to object" context menu item navigate to the resolved object.
    */
   public interface RowObjectResolver
   {
      /**
       * Get ID of the object related to given row.
       *
       * @param data table the row belongs to
       * @param row table row
       * @return object ID or 0 if there is no related object
       */
      public long getObjectId(Table data, TableRow row);
   }

   private final I18n i18n = LocalizationHelper.getI18n(TrafficQueryTable.class);

   private ObjectView view;
   private String configId;
   private String jobName;
   private TableQuery query;
   private RowObjectResolver objectResolver = null;
   private String noDataMessage = null;
   private String sortColumn = null;
   private int sortDirection = SWT.UP;
   private Table currentData = null;
   private SortableTableViewer viewer;
   private TableLabelProvider labelProvider;
   private CLabel messageLabel = null;
   private Action actionUseMultipliers;
   private Action actionGoToObject;

   /**
    * Create traffic query table.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    * @param configSubId configuration sub-ID for saved table settings
    * @param jobName name for the background read job
    * @param query data query
    */
   public TrafficQueryTable(Composite parent, int style, ObjectView view, String configSubId, String jobName, TableQuery query)
   {
      super(parent, style);
      this.view = view;
      this.configId = "TrafficQueryTable." + configSubId;
      this.jobName = jobName;
      this.query = query;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.setContentProvider(new TableContentProvider());
      labelProvider = new TableLabelProvider();
      labelProvider.setUseMultipliers(PreferenceStore.getInstance().getAsBoolean(configId + ".useMultipliers", false));
      viewer.setLabelProvider(labelProvider);
      viewer.addDoubleClickListener((e) -> goToObject());

      createActions();
      createContextMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionUseMultipliers = new Action(i18n.tr("Use &multipliers"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setUseMultipliers(actionUseMultipliers.isChecked());
            PreferenceStore.getInstance().set(configId + ".useMultipliers", actionUseMultipliers.isChecked());
            viewer.refresh(true);
         }
      };
      actionUseMultipliers.setChecked(labelProvider.areMultipliersUsed());

      actionGoToObject = new Action(i18n.tr("&Go to object")) {
         @Override
         public void run()
         {
            goToObject();
         }
      };
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));
      viewer.getControl().setMenu(menuMgr.createContextMenu(viewer.getControl()));
   }

   /**
    * Fill context menu
    *
    * @param manager menu manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      if (getSelectedObjectId() != 0)
      {
         manager.add(actionGoToObject);
         manager.add(new Separator());
      }
      manager.add(actionUseMultipliers);
   }

   /**
    * Get ID of the object related to currently selected row.
    *
    * @return object ID or 0 if there is no single selected row with known and accessible related object
    */
   private long getSelectedObjectId()
   {
      if ((objectResolver == null) || (currentData == null))
         return 0;

      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof TableRow))
         return 0;

      long objectId = objectResolver.getObjectId(currentData, (TableRow)selection.getFirstElement());
      return ((objectId != 0) && (Registry.getSession().findObjectById(objectId) != null)) ? objectId : 0;
   }

   /**
    * Go to object related to currently selected row
    */
   private void goToObject()
   {
      long objectId = getSelectedObjectId();
      if (objectId != 0)
         MainWindow.switchToObject(objectId, 0);
   }

   /**
    * Set resolver for objects related to table rows. Enables navigation to related object.
    *
    * @param objectResolver row object resolver
    */
   public void setObjectResolver(RowObjectResolver objectResolver)
   {
      this.objectResolver = objectResolver;
   }

   /**
    * Set message to display when the query returns no data.
    *
    * @param message message to display
    */
   public void setNoDataMessage(String message)
   {
      noDataMessage = message;
   }

   /**
    * Set initial sorting column and direction
    *
    * @param columnName name of the column to be used for initial sorting
    * @param direction initial sorting direction (SWT.UP or SWT.DOWN)
    */
   public void setSortColumn(String columnName, int direction)
   {
      sortColumn = columnName;
      sortDirection = direction;
   }

   /**
    * Get underlying table viewer.
    *
    * @return table viewer
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }

   /**
    * Get current table data.
    *
    * @return current table data or null
    */
   public Table getCurrentData()
   {
      return currentData;
   }

   /**
    * Refresh table
    *
    * @param postRefreshHook hook to run in UI thread after the viewer was updated (can be null)
    */
   public void refresh(final Runnable postRefreshHook)
   {
      Job job = new Job(jobName, view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Table table = query.read();
            runInUIThread(() -> {
               if (viewer.getControl().isDisposed())
                  return;

               if ((table == null) && (noDataMessage != null))
               {
                  showMessage(noDataMessage);
               }
               else
               {
                  hideMessage();
                  updateViewer(table);
               }
               if (postRefreshHook != null)
                  postRefreshHook.run();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot read traffic data (%s)"), jobName);
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Update viewer with fresh table data
    *
    * @param table new table data (null to clear the viewer)
    */
   private void updateViewer(final Table table)
   {
      if (table == null)
      {
         if (viewer.isInitialized())
         {
            currentData.deleteAllRows();
            viewer.refresh();
         }
         return;
      }

      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 150);
         int columnIndex = (sortColumn != null) ? table.getColumnIndex(sortColumn) : 0;
         if (columnIndex == -1)
            columnIndex = 0; // fallback to first column
         viewer.createColumns(names, widths, columnIndex, sortDirection);
         viewer.enablePersistence(configId);
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
         labelProvider.setColumns(table.getColumns());

         currentData = table;
         viewer.setInput(currentData);
      }
      else
      {
         currentData.deleteAllRows();
         currentData.addAll(table);
         viewer.refresh();
      }
   }

   /**
    * Show message instead of the table viewer
    *
    * @param message message to display
    */
   private void showMessage(String message)
   {
      if (messageLabel == null)
      {
         viewer.getControl().setVisible(false);
         ((GridData)viewer.getControl().getLayoutData()).exclude = true;
         messageLabel = new CLabel(this, SWT.CENTER);
         messageLabel.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));
      }
      messageLabel.setText(message);
      layout(true, true);
   }

   /**
    * Hide message and show the table viewer
    */
   private void hideMessage()
   {
      if (messageLabel != null)
      {
         messageLabel.dispose();
         messageLabel = null;
         ((GridData)viewer.getControl().getLayoutData()).exclude = false;
         viewer.getControl().setVisible(true);
         layout(true, true);
      }
   }
}
