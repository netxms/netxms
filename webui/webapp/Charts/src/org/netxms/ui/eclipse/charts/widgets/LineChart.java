/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;

/**
 * Line chart implementation
 */
public class LineChart extends GenericChart implements HistoricalDataChart {
	private static final long serialVersionUID = 1L;
	private List<GraphItemStyle> itemStyles;
	private boolean zoomEnabled;
	private boolean gridVisible;
	private List<GraphItem> items = new ArrayList<GraphItem>();
	private boolean showToolTips;
	private Map<Integer, double[]> data = new TreeMap<Integer, double[]>();
	private ChartColor backgroundColor;

	public LineChart(Composite parent, int style) {
		super(parent, style);
		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips");
		zoomEnabled = preferenceStore.getBoolean("Chart.EnableZoom");
		RGB color = PreferenceConverter.getColor(preferenceStore, "Chart.Colors.Background");
		setBackgroundColor(new ChartColor(color));
	}

	@Override
	public void initializationComplete() {
	}

	@Override
	public void setChartTitle(String title) {
		this.title = title;
	}

	@Override
	public void setTitleVisible(boolean visible) {
		System.out.println("titleVisible == " + visible);
		this.titleVisible = visible;
	}

	@Override
	public boolean isGridVisible() {
		return gridVisible;
	}

	@Override
	public void setGridVisible(boolean visible) {
		gridVisible = visible;
	}

	@Override
	public void setLegendVisible(boolean visible) {
		legendVisible = visible;
	}

	@Override
	public void set3DModeEnabled(boolean enabled) {
	}

	@Override
	public void setLogScaleEnabled(boolean enabled) {
	}

	@Override
	public void refresh() {
		this.redraw();
	}

	@Override
	public boolean hasAxes() {
		return true;
	}

	@Override
	public void setBackgroundColor(ChartColor color) {
		this.backgroundColor = color;
	}

	@Override
	public void setPlotAreaColor(ChartColor color) {
	}

	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background) {
	}

	@Override
	public void setAxisColor(ChartColor color) {
	}

	@Override
	public void setGridColor(ChartColor color) {
	}

	@Override
	public void setTimeRange(Date from, Date to) {
	}

	@Override
	public boolean isZoomEnabled() {
		return zoomEnabled;
	}

	@Override
	public void setZoomEnabled(boolean enableZoom) {
		zoomEnabled = enableZoom;
	}

	@Override
	public List<GraphItemStyle> getItemStyles() {
		return itemStyles;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setItemStyles(java.util.List)
	 */
	@Override
	public void setItemStyles(List<GraphItemStyle> itemStyles)
	{
		this.itemStyles = itemStyles;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#rebuild()
	 */
	@Override
	public void rebuild()
	{
		adjustYAxis(true);
	}

	@Override
	public int addParameter(GraphItem item) 
	{
		items.add(item);
		return items.size();
	}

	@Override
	public void updateParameter(int index, DciData data, boolean updateChart) 
	{
		if ((index < 0) || (index >= items.size())) 
		{
			return;
		}

		final DciDataRow[] values = data.getValues();

		final double[] flatData = new double[values.length * 2];
		for (int i = 0; i < values.length; i++) 
		{
			DciDataRow row = values[i];
			flatData[i * 2] = row.getTimestamp().getTime();
			flatData[(i * 2) + 1] = row.getValueAsDouble();
		}
		this.data.put(index, flatData);

		if (updateChart) 
		{
			refresh();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#adjustXAxis(boolean)
	 */
	@Override
	public void adjustXAxis(boolean repaint) 
	{
	}

	@Override
	public void adjustYAxis(boolean repaint) {
	}

	@Override
	public void zoomIn() {
	}

	@Override
	public void zoomOut() {
	}

	@Override
	public boolean isLegendVisible() {
		return legendVisible;
	}

	public List<String> getLegends() {
		final List<String> ret = new ArrayList<String>(items.size());
		for (GraphItem item : items) {
			ret.add(item.getDescription());
		}

		return ret;
	}

	public double[][] getFlatXYData() {
		return data.values().toArray(new double[][] {});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#addError(java.lang.String)
	 */
	@Override
	public void addError(String message)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#clearErrors()
	 */
	@Override
	public void clearErrors()
	{
		// TODO Auto-generated method stub
		
	}

	public String getBackgroundColor() {
		return "rgb(" + backgroundColor.red + "," + backgroundColor.green + "," + backgroundColor.blue + ")";
	}
}
