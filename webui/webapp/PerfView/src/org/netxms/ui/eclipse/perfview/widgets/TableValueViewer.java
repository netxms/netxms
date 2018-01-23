/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.views.DataComparisonView;
import org.netxms.ui.eclipse.perfview.views.HistoricalDataView;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Widget to show table DCI last value
 */
public class TableValueViewer extends BaseTableValueViewer
{
   private static long uniqueId = 1;

   private long objectId = 0;
   private long dciId = 0;
   private String objectName = null;
   private Action actionShowHistory;
   private Action actionShowLineChart;
   private Action actionShowBarChart;
   private Action actionShowPieChart;

   /**
    * @param parent
    * @param style
    * @param viewPart
    */
   public TableValueViewer(Composite parent, int style, IViewPart viewPart, String configSubId)
   {
      super(parent, style, viewPart, configSubId);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#buildConfigId(java.lang.String)
    */
   @Override
   protected String buildConfigId(String configSubId)
   {
      StringBuilder sb = new StringBuilder("TableLastValues."); //$NON-NLS-1$
      sb.append(dciId);
      if (configSubId != null)
      {
         sb.append('.');
         sb.append(configSubId);
      }
      return sb.toString();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();
      
      actionShowHistory = new Action("History", Activator.getImageDescriptor("icons/data_history.gif")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showHistory();
         }
      };
      
      actionShowLineChart = new Action(Messages.get().TableValue_LineChart, Activator.getImageDescriptor("icons/chart_line.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showLineChart();
         }
      };

      actionShowBarChart = new Action(Messages.get().TableValue_BarChart, Activator.getImageDescriptor("icons/chart_bar.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.BAR_CHART);
         }
      };

      actionShowPieChart = new Action(Messages.get().TableValue_PieChart, Activator.getImageDescriptor("icons/chart_pie.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.PIE_CHART);
         }
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionShowHistory);
      manager.add(actionShowLineChart);
      manager.add(actionShowBarChart);
      manager.add(actionShowPieChart);
      manager.add(new Separator());
      super.fillContextMenu(manager);
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
    * Show history
    */
   private void showHistory()
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;
      
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());

         String id = Long.toString(objectId) + "&" + Long.toString(dciId) + "@" //$NON-NLS-1$ //$NON-NLS-2$
               + safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")) + "@" //$NON-NLS-1$
               + safeEncode(instance) + "@" + safeEncode(column.getName());//$NON-NLS-1$ //$NON-NLS-2$
         
         final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
               .getActiveWorkbenchWindow().getActivePage();
         try
         {
            page.showView(HistoricalDataView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
                  String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
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

      StringBuilder sb = new StringBuilder();
      sb.append(uniqueId++);
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());
         
         sb.append("&");
         sb.append(ChartDciConfig.TABLE);
         sb.append("@");
         sb.append(objectId);
         sb.append("@");
         sb.append(dciId);
         sb.append("@");
         sb.append(safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")));
         sb.append("@");
         sb.append(safeEncode(instance));
         sb.append("@");
         sb.append(safeEncode(column.getName()));
      }

      final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
            .getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(HistoricalGraphView.ID, sb.toString(), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
               String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
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
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
               String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
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

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#readData()
    */
   @Override
   protected Table readData() throws Exception
   {
      return session.getTableLastValues(objectId, dciId);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#getReadJobName()
    */
   @Override
   protected String getReadJobName()
   {
      return String.format(Messages.get().TableValue_JobName, dciId);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.perfview.widgets.BaseTableValueViewer#getReadJobErrorMessage()
    */
   @Override
   protected String getReadJobErrorMessage()
   {
      return String.format(Messages.get().TableValue_JobError, dciId);
   }
}
