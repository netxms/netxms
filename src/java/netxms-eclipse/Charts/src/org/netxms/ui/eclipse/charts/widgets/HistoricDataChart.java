/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Activator;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.ILegend;
import org.swtchart.ILineSeries;
import org.swtchart.ISeriesSet;
import org.swtchart.ITitle;
import org.swtchart.LineStyle;
import org.swtchart.Range;
import org.swtchart.ILineSeries.PlotSymbolType;
import org.swtchart.ISeries.SeriesType;

/**
 * @author victor
 *
 */
public class HistoricDataChart extends Chart
{
	private long timeFrom;
	private long timeTo;
	private boolean showToolTips;
	private IPreferenceStore preferenceStore;
	
	/**
	 * @param parent
	 * @param style
	 */
	public HistoricDataChart(Composite parent, int style)
	{
		super(parent, style);
		
		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips");
		setBackground(getColorFromPreferences("Chart.Colors.Background"));

		// Setup title
		ITitle title = getTitle();
		title.setVisible(preferenceStore.getBoolean("Chart.ShowTitle"));
		title.setForeground(getColorFromPreferences("Chart.Colors.Title"));
		
		// Setup legend
		ILegend legend = getLegend();
		legend.setPosition(SWT.BOTTOM);
		
		// Default time range
		timeTo = System.currentTimeMillis();
		timeFrom = timeTo - 3600000;

		// Setup X and Y axis
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		xAxis.setRange(new Range(timeFrom, timeTo));
		IAxisTick xTick = xAxis.getTick();
		xTick.setForeground(getColorFromPreferences("Chart.Axis.X.Color"));
		DateFormat format = new SimpleDateFormat("HH:mm");
		xTick.setFormat(format);
		
		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
		yAxis.getTick().setForeground(getColorFromPreferences("Chart.Axis.Y.Color"));
		
		// Setup grid
		xAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.X.Style"));
		xAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.X.Color"));
		yAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.Y.Style"));
		yAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.Y.Color"));
		
		// Setup plot area
		setBackgroundInPlotArea(getColorFromPreferences("Chart.Colors.PlotArea"));
		final Composite plotArea = getPlotArea();
		if (showToolTips)
		{
			plotArea.addMouseTrackListener(new MouseTrackListener() {
				@Override
				public void mouseEnter(MouseEvent e)
				{
				}
	
				@Override
				public void mouseExit(MouseEvent e)
				{
				}
	
				@Override
				public void mouseHover(MouseEvent e)
				{
					Date timestamp = new Date((long)xAxis.getDataCoordinate(e.x));
					double value = yAxis.getDataCoordinate(e.y);
					getPlotArea().setToolTipText(DateFormat.getDateTimeInstance().format(timestamp) + "\n" + value);
				}
			});
		}
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
	 * Return line style object matching given label. If no match found, LineStyle.NONE is returned.
	 * 
	 * @param name Line style label
	 * @return Line style object
	 */
	private LineStyle getLineStyleFromPreferences(final String name)
	{
		String value = preferenceStore.getString(name);
		for(LineStyle s : LineStyle.values())
			if (s.label.equalsIgnoreCase(value))
				return s;
		return LineStyle.NONE;
	}

	/**
	 * Add line series to chart
	 * 
	 * @param description Description
	 * @param xSeries X axis data
	 * @param ySeries Y axis data
	 */
	public void addLineSeries(int index, String description, Date[] xSeries, double[] ySeries)
	{
		ISeriesSet seriesSet = getSeriesSet();
		ILineSeries series = (ILineSeries)seriesSet.createSeries(SeriesType.LINE, description);
		
		series.setAntialias(SWT.ON);
		series.setSymbolType(PlotSymbolType.NONE);
		series.setLineWidth(2);
		series.setLineColor(getColorFromPreferences("Chart.Colors.Data." + index));
		
		series.setXDateSeries(xSeries);
		series.setYSeries(ySeries);
	}
}
