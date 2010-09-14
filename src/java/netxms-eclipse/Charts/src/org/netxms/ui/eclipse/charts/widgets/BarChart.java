/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Activator;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.IBarSeries;
import org.swtchart.ILegend;
import org.swtchart.ISeriesSet;
import org.swtchart.ISeries.SeriesType;

/**
 * Bar chart for DCI comparision
 *
 */
public class BarChart extends Chart
{
	private IPreferenceStore preferenceStore;
	private boolean showToolTips;

	/**
	 * @param parent
	 * @param style
	 */
	public BarChart(Composite parent, int style)
	{
		super(parent, style);
		
		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips");
		setBackground(getColorFromPreferences("Chart.Colors.Background"));
		
		getTitle().setVisible(false);

		// Setup legend
		ILegend legend = getLegend();
		legend.setPosition(SWT.RIGHT);

		// Setup X and Y axis
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		IAxisTick xTick = xAxis.getTick();
		xTick.setForeground(getColorFromPreferences("Chart.Axis.X.Color"));

		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
		yAxis.getTick().setForeground(getColorFromPreferences("Chart.Axis.Y.Color"));

		// Setup plot area
		setBackgroundInPlotArea(getColorFromPreferences("Chart.Colors.PlotArea"));
	}

   /**
	 * Create color object from preference string
	 *  
	 * @param name Preference name
	 * @return Color object
	 */
	private Color getColorFromPreferences(final String name)
	{
		return new Color(getDisplay(), PreferenceConverter.getColor(preferenceStore, name));
	}

	/**
	 * Add series to chart
	 * 
	 * @param description Description
	 * @param xSeries X axis data
	 * @param ySeries Y axis data
	 */
	public IBarSeries addBarSeries(int index, String description, double value)
	{
		ISeriesSet seriesSet = getSeriesSet();
		IBarSeries series = (IBarSeries)seriesSet.createSeries(SeriesType.BAR, description);
		
		series.setBarColor(getColorFromPreferences("Chart.Colors.Data." + index));
		series.setBarPadding(25);

		series.setYSeries(new double[] { value });
		
		return series;
	}
	
	/**
	 * Enable categories on bar chart
	 * @param categories
	 */
	public void setCategories(String[] categories)
	{
		IAxis axis = this.getAxisSet().getXAxis(0);
		axis.enableCategory(true);
		axis.setCategorySeries(categories);
	}
}
