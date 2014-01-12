/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.widgets;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.views.DataComparisonView;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.perfview.widgets.helpers.CellSelectionManager;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableContentProvider;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableItemComparator;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Widget to show table DCI last value
 */
public class TableValue extends Composite
{
   private static long uniqueId = 1;

   private NXCSession session;
   private IViewPart viewPart;
   private long objectId = 0;
   private long dciId = 0;
   private String objectName = null;
   private Table currentData = null;
   private SortableTableViewer viewer;
   private CellSelectionManager cellSelectionManager;
   private Action actionShowLineChart;
   private Action actionShowBarChart;
   private Action actionShowPieChart;

   /**
    * @param parent
    * @param style
    * @param viewPart
    */
   public TableValue(Composite parent, int style, IViewPart viewPart)
   {
      super(parent, style);

      this.viewPart = viewPart;
      session = (NXCSession)ConsoleSharedData.getSession();

      setLayout(new FillLayout());

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new TableContentProvider());
      viewer.setLabelProvider(new TableLabelProvider());
      cellSelectionManager = new CellSelectionManager(viewer);

      createActions();
      createPopupMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionShowLineChart = new Action(Messages.TableValue_LineChart, Activator.getImageDescriptor("icons/chart_line.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showLineChart();
         }
      };

      actionShowBarChart = new Action(Messages.TableValue_BarChart, Activator.getImageDescriptor("icons/chart_bar.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.BAR_CHART);
         }
      };

      actionShowPieChart = new Action(Messages.TableValue_PieChart, Activator.getImageDescriptor("icons/chart_pie.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.PIE_CHART);
         }
      };
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

      // Register menu for extension
      if (viewPart != null)
      {
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
      }
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionShowLineChart);
      manager.add(actionShowBarChart);
      manager.add(actionShowPieChart);
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
   }

   /**
    * @param objectId
    * @param dciId
    */
   public void setObject(long objectId, long dciId)
   {
      this.objectId = objectId;
      this.dciId = dciId;
      objectName = session.getObjectName(objectId);
   }

   /**
    * Refresh table
    */
   public void refresh(final Runnable postRefreshHook)
   {
      viewer.setInput(null);
      new ConsoleJob(String.format(Messages.TableValue_JobName, dciId), viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Table table = session.getTableLastValues(objectId, dciId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  updateViewer(table);
                  if (postRefreshHook != null)
                  {
                     postRefreshHook.run();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.TableValue_JobError, dciId);
         }
      }.start();
   }

   /**
    * Update viewer with fresh table data
    * 
    * @param table table
    */
   private void updateViewer(final Table table)
   {
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 150);
         viewer.createColumns(names, widths, 0, SWT.UP);
         WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "TableLastValues"); //$NON-NLS-1$
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "TableLastValues"); //$NON-NLS-1$
            }
         });
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
      }
      ((TableLabelProvider)viewer.getLabelProvider()).setColumns(table.getColumns());
      viewer.setInput(table);
      currentData = table;
   }

   /**
    * @param viewerRow
    * @return
    */
   private String buildInstanceString(ViewerRow viewerRow)
   {
      StringBuilder instance = new StringBuilder();
      boolean first = true;
      for(int i = 0; i < currentData.getColumnCount(); i++)
      {
         TableColumnDefinition cd = currentData.getColumnDefinition(i);
         if (cd.isInstanceColumn())
         {
            if (!first)
               instance.append("~~~"); //$NON-NLS-1$
            instance.append(viewerRow.getText(i));
            first = false;
         }
      }
      return instance.toString();
   }

   /**
    * Show line chart
    */
   private void showLineChart()
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;

      String id = Long.toString(uniqueId++);
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());
         int source = currentData.getSource();

         id += "&" + Long.toString(objectId) + "@" + Long.toString(dciId) + "@" + Integer.toString(source) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
               + Integer.toString(column.getDataType()) + "@" + safeEncode(currentData.getTitle()) + "@" //$NON-NLS-1$ //$NON-NLS-2$
               + safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")) + "@" + safeEncode(instance) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
               + safeEncode(column.getName());
      }

      final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
            .getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(HistoricalGraphView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.TableValue_Error,
               String.format(Messages.TableValue_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }

   /**
    * Show line chart
    */
   private void showDataComparisonChart(int chartType)
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;

      String id = Long.toString(uniqueId++) + "&" + Integer.toString(chartType); //$NON-NLS-1$
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         String instance = buildInstanceString(cells[i].getViewerRow());
         int source = currentData.getSource();

         id += "&" + Long.toString(objectId) + "@" + Long.toString(dciId) + "@" + Integer.toString(source) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
               + Integer.toString(column.getDataType()) + "@" + safeEncode(currentData.getTitle()) + "@" //$NON-NLS-1$ //$NON-NLS-2$
               + safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")) + "@" + safeEncode(instance) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
               + safeEncode(column.getName());
      }

      final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
            .getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(DataComparisonView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.TableValue_Error,
               String.format(Messages.TableValue_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }

   /**
    * @param text
    * @return
    */
   private static String safeEncode(String text)
   {
      if (text == null)
         return ""; //$NON-NLS-1$

      try
      {
         return URLEncoder.encode(text, "UTF-8"); //$NON-NLS-1$
      }
      catch(UnsupportedEncodingException e)
      {
         return "none"; //$NON-NLS-1$
      }
   }

   /**
    * @return
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @return
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return
    */
   public String getObjectName()
   {
      return objectName;
   }

   /**
    * @return
    */
   public String getTitle()
   {
      return (currentData != null) ? currentData.getTitle() : ("[" + dciId + "]"); //$NON-NLS-1$ //$NON-NLS-2$
   }

   /**
    * @return
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }
}
