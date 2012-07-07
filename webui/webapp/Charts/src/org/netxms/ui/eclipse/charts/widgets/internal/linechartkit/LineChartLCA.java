package org.netxms.ui.eclipse.charts.widgets.internal.linechartkit;

import java.io.IOException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.rwt.lifecycle.AbstractWidgetLCA;
import org.eclipse.rwt.lifecycle.ControlLCAUtil;
import org.eclipse.rwt.lifecycle.IWidgetAdapter;
import org.eclipse.rwt.lifecycle.JSWriter;
import org.eclipse.rwt.lifecycle.WidgetLCAUtil;
import org.eclipse.rwt.lifecycle.WidgetUtil;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Widget;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.widgets.LineChart;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * @deprecated this whole Flot integration is a weird hack which should be replaced ASAP
 * @author alk
 * 
 */
@SuppressWarnings("deprecation")
public class LineChartLCA extends AbstractWidgetLCA
{

	@Override
	public void readData(Widget widget)
	{
		LineChart chart = (LineChart)widget;
	}

	@Override
	public void preserveValues(Widget widget)
	{
		ControlLCAUtil.preserveValues((Control)widget);
		IWidgetAdapter adapter = WidgetUtil.getAdapter(widget);
		// adapter.preserve( PROP_ADDRESS, ( ( GMap )widget ).getAddress() );

		// only needed for custom variants (themeing)
		WidgetLCAUtil.preserveCustomVariant(widget);
	}

	@Override
	public void renderInitialization(Widget widget) throws IOException
	{
		JSWriter writer = JSWriter.getWriterFor(widget);
		String id = WidgetUtil.getId(widget);
		writer.newWidget("org.netxms.ui.eclipse.charts.widgets.LineChart", new Object[] { id });
		writer.set("appearance", "composite");
		writer.set("overflow", "hidden");
		ControlLCAUtil.writeStyleFlags((LineChart)widget);
	}

	@Override
	public void renderChanges(Widget widget) throws IOException
	{
		LineChart chart = (LineChart)widget;
		ControlLCAUtil.writeChanges(chart);
		JSWriter writer = JSWriter.getWriterFor(widget);
		writer.set("gridVisible", chart.isGridVisible());
		writer.set("legendVisible", chart.isLegendVisible());
		writer.set("titleVisible", chart.isTitleVisible());
		writer.set("bgColor", chart.getBackgroundColor());
		writer.set("title", chart.getChartTitle());

		double[][] xyData = chart.getFlatXYData();
		if (xyData.length > 0)
		{
			DecimalFormat format = new DecimalFormat("###.###");
			Object[] arguments = new Object[xyData.length + 2];
			arguments[0] = chart.getLegends().toArray();
			List<GraphItemStyle> itemStyles = chart.getItemStyles();
			if (itemStyles == null)
			{
				itemStyles = getDefaultItemStyles();
			}
			List<String> colors = new ArrayList<String>(itemStyles.size());
			for(GraphItemStyle itemStyle : itemStyles)
			{
				int color = itemStyle.getColor();
				String textColor = "rgb(" + (color & 0xff) + "," + ((color & 0xff00) >> 8) + "," + ((color & 0xff0000) >> 16) + ")";
				colors.add(textColor);
			}
			arguments[1] = colors.toArray();

			for(int i = 0; i < xyData.length; i++)
			{
				final StringBuilder builder = new StringBuilder();
				double[] dciData = xyData[i];
				for(int j = 0; j < dciData.length; j++)
				{
					if (builder.length() > 0)
					{
						builder.append("|");
					}
					builder.append(format.format(dciData[j]));
				}
				arguments[i + 2] = builder.toString();
			}
			writer.call("update", arguments);
			// writer.call("update", null);
		}

		// writer.call("redraw", new Object[] {});

		// only needed for custom variants (themeing)
		WidgetLCAUtil.writeCustomVariant(widget);
	}

	private List<GraphItemStyle> getDefaultItemStyles()
	{
		List<GraphItemStyle> itemStyles = new ArrayList<GraphItemStyle>();
		IPreferenceStore preferenceStore = Activator.getDefault().getPreferenceStore();
		for(int i = 0; i < GraphSettings.MAX_GRAPH_ITEM_COUNT; i++)
		{
			itemStyles.add(new GraphItemStyle(GraphItemStyle.LINE, ColorConverter.getColorFromPreferencesAsInt(preferenceStore,
					"Chart.Colors.Data." + i), 0, 0));
		}
		return itemStyles;
	}

	@Override
	public void renderDispose(Widget widget) throws IOException
	{
		JSWriter writer = JSWriter.getWriterFor(widget);
		writer.dispose();
	}
}
