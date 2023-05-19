/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableCell;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.base.widgets.helpers.MenuContributionItem;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.helpers.ObjectSelectionProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SummaryTableContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SummaryTableItemComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SummaryTableItemLabelProvider;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.ObjectMenuFactory;
import org.netxms.nxmc.modules.objects.ObjectWrapper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI summary table display widget
 */
public class SummaryTableWidget extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryTableWidget.class);

   private int tableId;
   private long baseObjectId;
   private View view;
   private SortableTreeViewer viewer;
   private SummaryTableItemLabelProvider labelProvider;
   private Action actionExportToCsv;
   private Action actionUseMultipliers;
   private Action actionForcePollAll;
   private Action actionShowObjectDetails;
   private ViewRefreshController refreshController;
   private boolean useMultipliers = true;
   private TreeColumn currentColumn = null;
   private ObjectSelectionProvider objectSelectionProvider;
   private int showLineCount = 0;
   private List<String> sortingColumnList = null;

   /**
    * Create summary table widget
    * 
    * @param parent parent composite
    * @param style widget style
    * @param view view part this widget is associated with
    * @param tableId summary table ID
    * @param baseObjectId base object ID
    */
   public SummaryTableWidget(Composite parent, int style, View view, int tableId, long baseObjectId)
   {
      super(parent, style);
      
      this.view = view;
      this.tableId = tableId;
      this.baseObjectId = baseObjectId;
      
      setLayout(new FillLayout());

      viewer = new SortableTreeViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new SummaryTableContentProvider());
      labelProvider = new SummaryTableItemLabelProvider();
      labelProvider.setUseMultipliers(useMultipliers);
      viewer.setLabelProvider(labelProvider);

      objectSelectionProvider = new ObjectSelectionProvider(viewer);

      createActions();
      createContextMenu();

      refreshController = new ViewRefreshController(view, -1, new Runnable() {
         @Override
         public void run()
         {
            if (isDisposed())
               return;
            refresh();
         }
      });

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
      
      viewer.getTree().addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }
         
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 3)
               currentColumn = viewer.getColumnAtPoint(new Point(e.x, e.y));
         }
         
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });
   }

   /**
    * Create actions 
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(view, viewer, true);

      actionUseMultipliers = new Action(i18n.tr("Use &multipliers"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setUseMultipliers(!useMultipliers);
         }
      };
      actionUseMultipliers.setChecked(useMultipliers);

      actionForcePollAll = new Action(i18n.tr("&Force poll for all columns")) {
         @Override
         public void run()
         {
            forcePoll(true);
         }
      };

      actionShowObjectDetails = new Action(i18n.tr("Go to &object")) {
         @Override
         public void run()
         {
            showObjectDetails();
         }
      };
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager for underlying node object
      final MenuManager nodeMenuManager = new ObjectContextMenuManager(view, objectSelectionProvider, null) {
         @Override
         public String getMenuText()
         {
            return i18n.tr("&Node");
         }
      };

      // Create menu manager for rows
      MenuManager rowMenuManager = new MenuManager(view.getBaseId() + ".SummaryTableRowMenu." + this.hashCode());
      rowMenuManager.setRemoveAllWhenShown(true);
      rowMenuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr, nodeMenuManager);
         }
      });

      // Create menu.
      Menu menu = rowMenuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager, MenuManager nodeMenuManager)
   {
      manager.add(actionUseMultipliers);
      manager.add(new Separator());
      manager.add(nodeMenuManager);
      
      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      final Menu toolsMenu = ObjectMenuFactory.createToolsMenu(objectSelectionProvider.getStructuredSelection(), contextId, ((MenuManager)manager).getMenu(), null, new ViewPlacement(view));
      if (toolsMenu != null)
      {
         manager.add(new Separator());
         manager.add(new MenuContributionItem(i18n.tr("&Tools"), toolsMenu));
      }

      final Menu graphTemplatesMenu = ObjectMenuFactory.createGraphTemplatesMenu(objectSelectionProvider.getStructuredSelection(), contextId, ((MenuManager)manager).getMenu(), null, new ViewPlacement(view));
      if (graphTemplatesMenu != null)
      {
         manager.add(new Separator());
         manager.add(new MenuContributionItem(i18n.tr("&Graphs"), graphTemplatesMenu));
      }

      final Menu dashboardsMenu = ObjectMenuFactory.createDashboardsMenu(objectSelectionProvider.getStructuredSelection(), contextId, ((MenuManager)manager).getMenu(), null, new ViewPlacement(view));
      if (dashboardsMenu != null)
      {
         manager.add(new Separator());
         manager.add(new MenuContributionItem(i18n.tr("&Dashboards"), dashboardsMenu));
      }

      manager.add(new Separator());
      manager.add(actionShowObjectDetails);
      manager.add(new Separator());
      if ((currentColumn != null) && ((Integer)currentColumn.getData("ID") > 0)) //$NON-NLS-1$
      {
         manager.add(new Action(String.format(i18n.tr("Force poll for \"%s\""), currentColumn.getText())) {
            @Override
            public void run()
            {
               forcePoll(false);
            }
         });
      }
      manager.add(actionForcePollAll);
      manager.add(actionExportToCsv);
   }

   /**
    * Refresh table
    */
   public void refresh()
   {
      viewer.setInput(null);
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Reloading DCI summary table data"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Table table = session.queryDciSummaryTable(tableId, baseObjectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     update(table);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read data for DCI summary table");
         }
      };
      job.setUser(false);
      job.start();
   }
   
   /**
    * Update viewer with fresh table data
    * 
    * @param table table
    */
   public void update(final Table table)
   {
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 100);
         viewer.createColumns(names, widths, 0, SWT.UP);
         final PreferenceStore settings = PreferenceStore.getInstance();
         final String key = view.getBaseId() + ".SummaryTable." + Integer.toString(tableId);
         WidgetHelper.restoreTreeViewerSettings(viewer, key);
         useMultipliers = settings.getAsBoolean(key + ".useMultipliers", useMultipliers);
         labelProvider.setUseMultipliers(useMultipliers);
         viewer.getControl().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTreeViewerSettings(viewer, key);
               settings.set(key + ".useMultipliers", useMultipliers);
            }
         });
         viewer.setComparator(new SummaryTableItemComparator(table));
      }

      labelProvider.setColumnDataTypes(table.getColumns());
      if ((sortingColumnList != null) && !sortingColumnList.isEmpty())
      {
         List<SortItem> sortItem = new ArrayList<SortItem>();
         for(int i = 0; i < sortingColumnList.size() ; i++)
         {            
            boolean isDesc = sortingColumnList.get(i).charAt(0) == '>' ? true : false;
            int index = table.getColumnIndex(sortingColumnList.get(i).substring(1));
            if(index >= 0)
            {
               sortItem.add(new SortItem(index, isDesc));
            }
         }
         
         // find index of columns to compare and desc or asc
         final List<SortItem> sortItemFin = sortItem;
         table.sort(new Comparator<TableRow>() {
            public int compare(TableRow row1, TableRow row2)
            {
               //compare lines
               int result = 0;
               int i = 0;
               while(result == 0 && i < sortItemFin.size())
               {
                  result = compareItem(row1, row2, sortItemFin.get(i).columnIndex, sortItemFin.get(i).descending);
                  i++;
               }
               return result;
            }
            
            private int compareItem(TableRow row1, TableRow row2, int index, boolean sortDesc)
            {
               TableCell c1 = row1.get(index);
               TableCell c2 = row2.get(index);

               String s1 = (c1 != null) ? c1.getValue() : "";
               String s2 = (c2 != null) ? c2.getValue() : "";
               
               int result = 0;
               try
               {
                  double value1 = Double.parseDouble(s1);
                  double value2 = Double.parseDouble(s2);
                  result = Double.compare(value1, value2);
               }
               catch(NumberFormatException e)
               {
                  result = s1.compareToIgnoreCase(s2);
               }
               return sortDesc ? -result : result;
            }
         });
         viewer.setComparator(null);
      }

      if (showLineCount > 0)
         viewer.setInput(table.getFirstRows(showLineCount));
      else
         viewer.setInput(table);
      viewer.expandAll();
   }

   /**
    * Class for advanced column sorting
    */
   class SortItem
   {
      public int columnIndex;
      public boolean descending;
      
      public SortItem(int columnIndex, boolean descending)
      {
         this.columnIndex = columnIndex;
         this.descending = descending;
      }
   };

   /**
    * @return the viewer
    */
   public SortableTreeViewer getViewer()
   {
      return viewer;
   }

   /**
    * @return
    */
   public ISelectionProvider getObjectSelectionProvider()
   {
      return objectSelectionProvider;
   }

   /**
    * Set automatic refresh interval. If less or equal 0, automatic refresh will be disabled.
    * 
    * @param interval auto refresh interval in seconds
    */
   public void setAutoRefresh(int interval)
   {
      refreshController.setInterval(interval);
   }

   /**
    * @return
    */
   public boolean areMultipliersUsed()
   {
      return useMultipliers;
   }

   /**
    * @param value
    */
   public void setUseMultipliers(boolean value)
   {
      useMultipliers = value;
      actionUseMultipliers.setChecked(value);
      if (viewer.isInitialized())
      {
         labelProvider.setUseMultipliers(value);
         viewer.refresh();
      }
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }

   /**
    * Show details for selected object
    */
   private void showObjectDetails()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      AbstractObject object = ((ObjectWrapper)selection.getFirstElement()).getObject();
      MainWindow.switchToObject(object.getObjectId(), 0);
   }

   /**
    * @param pollAll
    */
   private void forcePoll(boolean pollAll)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      final List<PollRequest> requests = new ArrayList<PollRequest>();
      for(Object o : selection.toList())
      {
         TableRow r = (TableRow)o;
         long nodeId = r.getObjectId();
         if (pollAll)
         {
            int count = ((Table)viewer.getInput()).getColumnCount();
            for(int i = 1; i < count; i++)
            {
               long dciId = r.get(i).getObjectId();
               if (dciId != 0)
               {
                  requests.add(new PollRequest(nodeId, dciId));
               }
            }
         }
         else
         {
            int index = ((Table)viewer.getInput()).getColumnIndex(currentColumn.getText());
            long dciId = r.get(index).getObjectId();
            if (dciId != 0)
            {
               requests.add(new PollRequest(nodeId, dciId));
            }
         }
      }
      
      if (requests.isEmpty())
         return;
      
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Running forced DCI poll"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), requests.size());
            for(PollRequest r : requests)
            {
               session.forceDCIPoll(r.nodeId, r.dciId);
               monitor.worked(1);
            }
            monitor.done();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Forced DCI poll failed");
         }
      }.start();
   }
   
   /**
    * Forced poll request
    */
   private class PollRequest
   {
      public long nodeId;
      public long dciId;
      
      public PollRequest(long nodeId, long dciId)
      {
         this.nodeId = nodeId;
         this.dciId = dciId;
      }
   }
   
   /**
    * Set number of lines to show
    * 
    * @param lineCount line count to be shown
    */
   public void setShowNumLine(int lineCount)
   {
      this.showLineCount = lineCount;
   }

   /**
    * Set columns that will be used to sort table
    * 
    * @param sortingColumnList columns to be used while sorting
    */
   public void setSortColumns(List<String> sortingColumnList)
   {
      this.sortingColumnList = sortingColumnList;      
   }
}
