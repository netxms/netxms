/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredViewer;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.datacollection.views.DataComparisonView;
import org.netxms.nxmc.modules.datacollection.views.HistoricalDataView;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.modules.datacollection.views.TableLastValuesView;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ShowHistoricalDataMenuItems
{
   private final I18n i18n = LocalizationHelper.getI18n(ShowHistoricalDataMenuItems.class);

   private View view;
   private StructuredViewer viewer;
   private AbstractObject parent;

   private Action actionShowHistory;
   private Action actionRawLineChart;
   private Action actionShowLineChart;
   private Action actionShowBarChart;
   private Action actionShowPieChart;
   private Action actionShowTableData;

   /**
    * Populate given menu with historical data access menu items.
    *
    * @param manager menu manager
    * @param view owning view
    * @param object current object
    * @param viewer owning viewer
    * @param selectionType selection type
    */
   public static void populateMenu(IMenuManager manager, View view, AbstractObject object, StructuredViewer viewer, int selectionType)
   {
      ShowHistoricalDataMenuItems items = new ShowHistoricalDataMenuItems(view, object, viewer);
      if (selectionType == DataCollectionObject.DCO_TYPE_ITEM)
      {
         manager.add(items.actionShowHistory);
         manager.add(items.actionRawLineChart);
         manager.add(items.actionShowLineChart);
         manager.add(items.actionShowBarChart);
         manager.add(items.actionShowPieChart);
         manager.add(new Separator());   
      }
      if (selectionType == DataCollectionObject.DCO_TYPE_TABLE)
      {
         manager.add(items.actionShowTableData);   
         manager.add(new Separator());      
      }
   }

   /**
    * Create new menu manager for object's "Create" menu.
    *
    * @param shell parent shell
    * @param view parent view
    * @param parent object to create menu for
    */
   private ShowHistoricalDataMenuItems(View view, AbstractObject parent, StructuredViewer viewer)
   {
      this.view = view;
      this.viewer = viewer;
      this.parent = parent;   
      createActions();
    }

   /**
    * Create object actions
    */
   protected void createActions()
   {
      
      actionShowHistory = new Action("History", ResourceManager.getImageDescriptor("icons/object-views/history-view.png")) { 
         @Override
         public void run()
         {
            showHistoryData();
         }
      };
      view.addKeyBinding("M1+H", actionShowHistory);
      
      actionShowLineChart = new Action(i18n.tr("&Line chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png")) { 
         @Override
         public void run()
         {
            showLineChart(false);
         }
      };
      view.addKeyBinding("M1+L", actionShowLineChart);

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

      actionRawLineChart = new Action(i18n.tr("&Raw data line chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png")) {
         @Override
         public void run()
         {
            showLineChart(true);
         }
      };
      view.addKeyBinding("M1+M2+L", actionRawLineChart);

      actionShowTableData = new Action(i18n.tr("Table last value"), ResourceManager.getImageDescriptor("icons/object-views/table-value.png")) {
         @Override
         public void run()
         {
            showHistoryData();
         }
      };
   }
   
   /**
    * Get DCI id
    */
   protected long getDciId(Object dci)
   {
      return dci instanceof DataCollectionObject ? ((DataCollectionObject)dci).getId() : ((DciValue)dci).getId();
   }

   /**
    * Get object id
    */
   protected long getObjectId(Object dci)
   {
      return dci instanceof DataCollectionObject ? ((DataCollectionObject)dci).getNodeId() : ((DciValue)dci).getNodeId();
   }
   
   /**
    * Show line chart for selected items
    */
   private void showHistoryData()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      for(Object dcObject : selection.toList())
      {
         if ((dcObject instanceof DataCollectionTable) || ((dcObject instanceof DciValue) &&
               ((DciValue)dcObject).getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE))
         {
            view.openView(new TableLastValuesView(parent, getObjectId(dcObject), getDciId(dcObject)));
         }
         else 
         {
            view.openView(new HistoricalDataView(parent, getObjectId(dcObject), getDciId(dcObject)));
         }
      }
   }

   /**
    * Get chart configuration form object
    * @param o object
    * @return chart configuration
    */
   protected ChartDciConfig getConfigFromObject(Object o)
   {
      if (o instanceof DataCollectionObject)
         return new ChartDciConfig((DataCollectionObject)o);
      else
         return new ChartDciConfig((DciValue)o);
   }

   /**
    * Show line chart for selected items
    */
   private void showLineChart(boolean useRawValues)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      List<ChartDciConfig> items = new ArrayList<ChartDciConfig>(selection.size());
      for(Object o : selection.toList())
      {
         ChartDciConfig config = getConfigFromObject(o);
         config.useRawValues = useRawValues;
         items.add(config);
      }

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new HistoricalGraphView(parent, items, null, contextId));
   }

   /**
    * Show line chart
    */
   private void showDataComparisonChart(ChartType chartType)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      ArrayList<ChartDciConfig> items = new ArrayList<ChartDciConfig>(selection.size());
      for(Object o : selection.toList())
      {
         items.add(new ChartDciConfig((DciValue)o));
      }

      long contextId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
      view.openView(new DataComparisonView(parent.getObjectId(), items, chartType, contextId));
   }   
}
