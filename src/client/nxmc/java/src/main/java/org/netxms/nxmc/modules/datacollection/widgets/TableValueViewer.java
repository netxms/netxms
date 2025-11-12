/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.datacollection.views.DataComparisonView;
import org.netxms.nxmc.modules.datacollection.views.HistoricalDataView;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Widget to show table DCI last value
 */
public class TableValueViewer extends BaseTableValueViewer
{
   private I18n i18n;
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
    * @param view
    * @param configSubId
    * @param saveTableSettings
    */
   public TableValueViewer(Composite parent, int style, ObjectView view, String configSubId, boolean saveTableSettings)
   {
      super(parent, style, view, configSubId, saveTableSettings);
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(TableValueViewer.class);
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#buildConfigId(java.lang.String)
    */
   @Override
   protected String buildConfigId(String configSubId)
   {
      StringBuilder sb = new StringBuilder("TableLastValues."); 
      sb.append(dciId);
      if (configSubId != null)
      {
         sb.append('.');
         sb.append(configSubId);
      }
      return sb.toString();
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();

      actionShowHistory = new Action(i18n.tr("History"), ResourceManager.getImageDescriptor("icons/object-views/history-view.png")) { 
         @Override
         public void run()
         {
            showHistory();
         }
      };
      
      actionShowLineChart = new Action(i18n.tr("&Line chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png")) { 
         @Override
         public void run()
         {
            showLineChart();
         }
      };

      actionShowBarChart = new Action(i18n.tr("&Bar chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-bar.png")) { 
         @Override
         public void run()
         {
            showDataComparisonChart(ChartType.BAR);
         }
      };

      actionShowPieChart = new Action(i18n.tr("&Pie chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-pie.png")) { 
         @Override
         public void run()
         {
            showDataComparisonChart(ChartType.PIE);
         }
      };
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#fillContextMenu(org.eclipse.jface.action.IMenuManager)
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
         final String tableName = column.getDisplayName() + ": " + instance.replace("~~~", " / ");
         AbstractObject object = session.findObjectById(objectId);
         view.openView(new HistoricalDataView(object, object.getObjectId(), dciId, tableName, instance, column.getName()));
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
      List<ChartDciConfig> items = new ArrayList<ChartDciConfig>(cells.length);
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());
         
         ChartDciConfig config = new ChartDciConfig();
         config.nodeId = objectId;
         config.dciId = dciId;
         config.dciName = column.getDisplayName() + ": " + instance.replace("~~~", " / ");
         config.type = ChartDciConfig.TABLE;
         config.instance = instance;
         config.column = column.getName();
         items.add(config);
         
      }

      AbstractObject object = view.getObject();
      view.openView(new HistoricalGraphView(object, items, null, 0));
   }

   /**
    * Show line chart
    */
   private void showDataComparisonChart(ChartType chartType)
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;

      List<ChartDciConfig> items = new ArrayList<>(cells.length);
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         String instance = buildInstanceString(cells[i].getViewerRow());

         ChartDciConfig item = new ChartDciConfig();
         item.nodeId = objectId;
         item.dciId = dciId;
         item.dciName = currentData.getTitle();
         item.name = column.getDisplayName() + ": " + instance.replace("~~~", " / ");
         item.instance = instance;
         item.column = column.getName();
         item.displayFormat = "%s";
         items.add(item);
      }

      view.openView(new DataComparisonView(objectId, items, chartType, objectId));
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
      return (currentData != null) ? currentData.getTitle() : ("[" + dciId + "]");  
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#readData()
    */
   @Override
   protected Table readData() throws Exception
   {
      if (objectId == 0)
         return null;
      return session.getTableLastValues(objectId, dciId);
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobName()
    */
   @Override
   protected String getReadJobName()
   {
      return String.format(i18n.tr("Loading data for table DCI %d"), dciId);
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobErrorMessage()
    */
   @Override
   protected String getReadJobErrorMessage()
   {
      return String.format(i18n.tr("Cannot get data for table DCI %d"), dciId);
   }

   /**
    * Reset columns to default state
    */
   public void resetColumns()
   {
      if (currentData == null)
         return;
      currentData.deleteAllRows();
      viewer.reset();
   }
}
