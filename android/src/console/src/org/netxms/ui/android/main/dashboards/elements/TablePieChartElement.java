/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.HashMap;
import java.util.Map;
import org.achartengine.ChartFactory;
import org.achartengine.model.CategorySeries;
import org.achartengine.renderer.DefaultRenderer;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.netxms.client.Table;
import org.netxms.ui.android.main.dashboards.configs.TablePieChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;

/**
 * Pie chart element for table DCI
 */
public class TablePieChartElement extends AbstractDashboardElement
{
	private static final String LOG_TAG = "nxclient/TablePieChartElement";
	
	private TablePieChartConfig config;
	private CategorySeries dataset;
	private Map<String, Integer> instanceMap = new HashMap<String, Integer>(MAX_CHART_ITEMS);
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public TablePieChartElement(Context context, String xmlConfig, ClientConnectorService service)
	{
		super(context, xmlConfig, service);
		try
		{
			config = TablePieChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(LOG_TAG, "Error parsing element config", e);
			config = new TablePieChartConfig();
		}
		
		dataset = buildDataset();
		//chart = new PieChart(dataset, buildRenderer());
		
		startRefreshTask(config.getRefreshRate());
	}

	/**
	 * @return
	 */
	private CategorySeries buildDataset()
	{
		CategorySeries series = new CategorySeries(config.getTitle());

		return series;
	}
	
	/**
	 * @return
	 */
	private DefaultRenderer buildRenderer(int count)
	{
		DefaultRenderer renderer = new DefaultRenderer();
		renderer.setLabelsTextSize(15);
		renderer.setLegendTextSize(15);
		renderer.setMargins(new int[] { 20, 30, 15, 0 });
		renderer.setPanEnabled(false);
		renderer.setZoomEnabled(false);
		for(int i = 0; i < count; i++)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
			r.setColor(DEFAULT_ITEM_COLORS[i] | 0xFF000000);
			renderer.addSeriesRenderer(r);
		}
		
		renderer.setApplyBackgroundColor(true);
		renderer.setBackgroundColor(BACKGROUND_COLOR);
		
		return renderer;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement#refresh()
	 */
	@Override
	public void refresh()
	{
		try
		{
			final Table data = service.getSession().getTableLastValues(config.getNodeId(), config.getDciId());
			
			post(new Runnable() {
				@Override
				public void run()
				{
					String instanceColumn = (config.getInstanceColumn() != null) ? config.getInstanceColumn() : data.getInstanceColumn();
					if (instanceColumn == null)
						return;
					
					int icIndex = data.getColumnIndex(instanceColumn);
					int dcIndex = data.getColumnIndex(config.getDataColumn());
					if ((icIndex == -1) || (dcIndex == -1))
						return;	// at least one column is missing
					
					for(int i = 0; i < data.getRowCount(); i++)
					{
						String instance = data.getCell(i, icIndex);
						if (instance == null)
							continue;

						double value;
						try
						{
							value = Double.parseDouble(data.getCell(i, dcIndex));
						}
						catch(NumberFormatException e)
						{
							value = 0.0;
						}
						
						Integer index = instanceMap.get(instance);
						if (config.isIgnoreZeroValues() && (value == 0) && (index == null))
							continue;

						if (index != null)
						{
							dataset.set(index, instance, value);
						}
						else
						{
							index = dataset.getItemCount();
							if (index < MAX_CHART_ITEMS)
							{
								instanceMap.put(instance, index);
								dataset.add(instance, value);
							}
						}
					}					
					
					removeAllViews();
					addView(ChartFactory.getPieChartView(getContext(), dataset, buildRenderer(dataset.getItemCount())));
				}
			});
		}
		catch(Exception e)
		{
			Log.e("nxclient/BarChartElement", "Exception while reading data from server", e);
		}
	}
}
