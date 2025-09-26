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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Widget;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.simpleframework.xml.ElementList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * View for comparing DCI values visually using charts.
 */
public class DataComparisonView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(DataComparisonView.class);
   private static final Logger logger = LoggerFactory.getLogger(DataComparisonView.class);

   private Chart chart;
	private boolean updateInProgress = false;
   private List<ChartDciConfig> items = new ArrayList<>(8);
	private ViewRefreshController refreshController;
	private boolean autoRefreshEnabled = true;
	private boolean useLogScale = false;
	private int autoRefreshInterval = 30;	// 30 seconds
   private ChartType chartType = ChartType.BAR;
	private boolean transposed = false;
	private boolean showLegend = true;
	private int legendPosition = ChartConfiguration.POSITION_BOTTOM;
	private boolean translucent = false;
   private Image[] titleImages = new Image[4];

	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;
	private Action actionShowBarChart;
	private Action actionShowPieChart;
	private Action actionShowTranslucent;
	private Action actionUseLogScale;
	private Action actionHorizontal;
	private Action actionVertical;
	private Action actionShowLegend;
	private Action actionLegendLeft;
	private Action actionLegendRight;
	private Action actionLegendTop;
	private Action actionLegendBottom;
	private Action actionCopyImage;
	private Action actionSaveAsImage;

   /**
    * Build view ID
    *
    * @param object context object
    * @param items list of DCIs to show
    * @return view ID
    */
   private static String buildId(List<ChartDciConfig> items, ChartType type)
   {
      StringBuilder sb = new StringBuilder("DataComparisonView");
      sb.append('#');
      sb.append(type);
      for(ChartDciConfig item : items)
      {
         sb.append('#');
         sb.append(item.getNodeId());
         sb.append('#');
         sb.append(item.getDciId());
      }
      return sb.toString();
   }  

   /**
    * Create data comparison view.
    *
    * @param objectId context object ID
    * @param items graph items
    * @param chartType chart type
    */
   public DataComparisonView(long objectId, List<ChartDciConfig> items, ChartType chartType, long contextId)
	{      
      super(LocalizationHelper.getI18n(DataComparisonView.class).tr("Last Values Chart"),
            ResourceManager.getImageDescriptor((chartType == ChartType.PIE) ? "icons/object-views/chart-pie.png" : "icons/object-views/chart-bar.png"), buildId(items, chartType), objectId, contextId, false);
 
      this.chartType = chartType;
      this.items = items;
	}

   /**
    * Default constructor for view clone
    */
   protected DataComparisonView()
   {      
      super(LocalizationHelper.getI18n(DataComparisonView.class).tr("Last Values Chart"), null, null, 0, 0, false); 
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      DataComparisonView view = (DataComparisonView)super.cloneView();
      view.chartType = chartType;
      view.items = items;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setLegendPosition(legendPosition);
      chartConfiguration.setLegendVisible(showLegend);
      chartConfiguration.setTransposed(transposed);
      chartConfiguration.setTranslucent(translucent);

      chart = new Chart(parent, SWT.NONE, chartType, chartConfiguration, this);
      for(ChartDciConfig item : items)
         chart.addParameter(item);
      chart.rebuild();

      createActions();
		createContextMenu();

		updateChart();

		refreshController = new ViewRefreshController(this, autoRefreshEnabled ? autoRefreshInterval : -1, new Runnable() {
			@Override
			public void run()
			{
				if (((Widget)chart).isDisposed())
					return;

				updateChart();
			}
		});
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				updateChart();
			}
		};

		actionAutoRefresh = new Action(i18n.tr("Refresh &automatically")) {
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				refreshController.setInterval(autoRefreshEnabled ? autoRefreshInterval : -1);
			}
		};
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		
		actionUseLogScale = new Action(i18n.tr("&Logarithmic scale")) {
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
            chart.getConfiguration().setLogScale(useLogScale);
            chart.rebuild();
			}
		};
		actionUseLogScale.setChecked(useLogScale);
		
		actionShowTranslucent = new Action(i18n.tr("T&ranslucent")) {
			@Override
			public void run()
			{
				translucent = !translucent;
				setChecked(translucent);
            chart.getConfiguration().setTranslucent(translucent);
            chart.rebuild();
			}
		};
		actionShowTranslucent.setChecked(translucent);
		
		actionShowLegend = new Action(i18n.tr("&Show legend")) {
			@Override
			public void run()
			{
				showLegend = !showLegend;
				setChecked(showLegend);
            chart.getConfiguration().setLegendVisible(showLegend);
            chart.rebuild();
			}
		};
		actionShowLegend.setChecked(showLegend);
		
		actionLegendLeft = new Action(i18n.tr("Place on &left"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_LEFT;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendLeft.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionLegendRight = new Action(i18n.tr("Place on &right"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_RIGHT;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendRight.setChecked(legendPosition == ChartConfiguration.POSITION_RIGHT);
		
		actionLegendTop = new Action(i18n.tr("Place on &top"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_TOP;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendTop.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionLegendBottom = new Action(i18n.tr("Place on &bottom"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_BOTTOM;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendBottom.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionShowBarChart = new Action(i18n.tr("&Bar chart"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setChartType(ChartType.BAR);
			}
		};
      actionShowBarChart.setChecked(chartType == ChartType.BAR);
		actionShowBarChart.setImageDescriptor(ResourceManager.getImageDescriptor("icons/object-views/chart-bar.png")); 
		
      actionShowPieChart = new Action(i18n.tr("&Pie chart"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setChartType(ChartType.PIE);
			}
		};
      actionShowPieChart.setChecked(chartType == ChartType.PIE);
		actionShowPieChart.setImageDescriptor(ResourceManager.getImageDescriptor("icons/object-views/chart-pie.png")); 

		actionHorizontal = new Action(i18n.tr("Show &horizontally"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				transposed = true;
            chart.getConfiguration().setTransposed(true);
            chart.rebuild();
			}
		};
		actionHorizontal.setChecked(transposed);
		actionHorizontal.setEnabled(chart.hasAxes());
      actionHorizontal.setImageDescriptor(ResourceManager.getImageDescriptor("icons/object-views/chart-bar-horizontal.png"));
		
		actionVertical = new Action(i18n.tr("Show &vertically"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				transposed = false;
            chart.getConfiguration().setTransposed(false);
            chart.rebuild();
			}
		};
		actionVertical.setChecked(!transposed);
		actionVertical.setEnabled(chart.hasAxes());
      actionVertical.setImageDescriptor(ResourceManager.getImageDescriptor("icons/object-views/chart-bar.png"));

      actionCopyImage = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            chart.copyToClipboard();
         }
      };
      addKeyBinding("M1+C", actionCopyImage);

      actionSaveAsImage = new Action("Save as &image", SharedIcons.SAVE_AS_IMAGE) {
			@Override
			public void run()
			{
            chart.saveAsImage(getWindow().getShell());
			}
		};
      addKeyBinding("M1+I", actionSaveAsImage);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSaveAsImage);
      manager.add(actionCopyImage);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      MenuManager legend = new MenuManager(i18n.tr("&Legend"));
      legend.add(actionShowLegend);
      legend.add(new Separator());
      legend.add(actionLegendLeft);
      legend.add(actionLegendRight);
      legend.add(actionLegendTop);
      legend.add(actionLegendBottom);

      manager.add(actionShowBarChart);
      manager.add(actionShowPieChart);
      manager.add(new Separator());
      manager.add(actionVertical);
      manager.add(actionHorizontal);
      manager.add(new Separator());
      manager.add(actionShowTranslucent);
      manager.add(actionUseLogScale);
      manager.add(actionAutoRefresh);
      manager.add(legend);
      manager.add(new Separator());
      manager.add(actionCopyImage);
      manager.add(actionSaveAsImage);
   }

   /**
    * Create pop-up menu for user list
    */
   private void createContextMenu()
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
      Menu menu = menuMgr.createContextMenu(chart);
      chart.setMenu(menu);
   }

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		MenuManager legend = new MenuManager(i18n.tr("&Legend"));
		legend.add(actionShowLegend);
		legend.add(new Separator());
		legend.add(actionLegendLeft);
		legend.add(actionLegendRight);
		legend.add(actionLegendTop);
		legend.add(actionLegendBottom);
		
		manager.add(actionShowBarChart);
		manager.add(actionShowPieChart);
		manager.add(new Separator());
		manager.add(actionVertical);
		manager.add(actionHorizontal);
		manager.add(new Separator());
		manager.add(actionShowTranslucent);
		manager.add(actionUseLogScale);
		manager.add(actionAutoRefresh);
		manager.add(legend);
		manager.add(new Separator());
		manager.add(actionRefresh);
      manager.add(new Separator());
      manager.add(actionCopyImage);
      manager.add(actionSaveAsImage);  
	}

	/**
	 * Set new chart type
	 * @param newType
	 */
   private void setChartType(ChartType newType)
	{
		chartType = newType;
      chart.getConfiguration().setLabelsVisible(chartType == ChartType.PIE);
      chart.setType(chartType);
      chart.rebuild();

		actionHorizontal.setEnabled(chart.hasAxes());
		actionVertical.setEnabled(chart.hasAxes());
		try
		{
         // FIXME: setTitleImage(getIconByChartType(chartType));
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
		}
	}

	/**
	 * Get DCI data from server
	 */
	private void updateChart()
	{
		if (updateInProgress)
			return;

		updateInProgress = true;
		Job job = new Job(i18n.tr("Get DCI values for chart"), this) {
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get DCI data");
			}

			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final double[] values = new double[items.size()];
				for(int i = 0; i < items.size(); i++)
				{
               ChartDciConfig item = items.get(i);
               DataSeries data = (item.type == DataCollectionObject.DCO_TYPE_ITEM) ?
							session.getCollectedData(item.getNodeId(), item.getDciId(), null, null, 1, HistoricalDataType.PROCESSED) :
                     session.getCollectedTableData(item.getNodeId(), item.getDciId(), item.instance, item.column, null, null, 1);
					DciDataRow value = data.getLastValue();
					values[i] = (value != null) ? value.getValueAsDouble() : 0.0;
				}

            runInUIThread(() -> {
               setChartData(values);
               clearMessages();
               updateInProgress = false;
				});
			}

			@Override
			protected void jobFailureHandler(Exception e)
			{
            updateInProgress = false;
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * Set new data for chart items
	 * 
	 * @param values new values
	 */
	private void setChartData(double[] values)
	{
		for(int i = 0; i < values.length; i++)
			chart.updateParameter(i, values[i], false);
		chart.refresh();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      refreshController.dispose();
      for(Image i : titleImages)
      {
         if (i != null)
            i.dispose();
      }
      super.dispose();
   }
   
   /**
    * Wrapper class for items ArrayList
    */
   static class ItemWrapper {

      @ElementList(name="items")
      private List<ChartDciConfig> items = new ArrayList<ChartDciConfig>();

      public ItemWrapper() { }

      public ItemWrapper(List<ChartDciConfig> items) {
          this.items = items;
      }

      public List<ChartDciConfig> getData() {
          return this.items;
      }
  }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("chartType", chartType.toString());
      try
      {
         memento.set("configuration", XMLTools.serialize(new ItemWrapper(items)));
      }
      catch(Exception e)
      {
         logger.error("Failed to serialize configuration", e);
         memento.set("configuration", "");
      }
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      chartType = ChartType.valueOf(memento.getAsString("chartType"));
      try
      {         
         items = XMLTools.createFromXml(ItemWrapper.class, memento.getAsString("configuration")).getData();
      }
      catch(Exception e)
      {
         logger.error("Failed to load configuration", e);
         throw(new ViewNotRestoredException(i18n.tr("Failed to load configuration"), e));
      }
      
      setImage(ResourceManager.getImageDescriptor((chartType == ChartType.PIE) ? "icons/object-views/chart-pie.png" : "icons/object-views/chart-bar.png"));
   }
}
