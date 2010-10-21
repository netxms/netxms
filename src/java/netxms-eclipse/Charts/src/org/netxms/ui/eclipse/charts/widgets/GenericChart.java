package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataChart;

public abstract class GenericChart extends Canvas implements DataChart
{
	protected String title = "Chart";
	protected boolean titleVisible = false;
	protected boolean legendVisible = true;
	protected boolean displayIn3D = true;
	protected ChartColor[] palette = null;
	protected IPreferenceStore preferenceStore;

	/**
	 * Generic constructor.
	 * 
	 * @param parent parent composite
	 * @param style SWT control styles
	 */
	public GenericChart(Composite parent, int style)
	{
		super(parent, style);

		preferenceStore = Activator.getDefault().getPreferenceStore();
		createDefaultPalette();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getTitle()
	 */
	@Override
	public String getTitle()
	{
		return title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLegendVisible()
	 */
	@Override
	public boolean isLegendVisible()
	{
		return legendVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTitleVisible()
	 */
	@Override
	public boolean isTitleVisible()
	{
		return titleVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPalette(org.netxms.ui.eclipse.charts.api.ChartColor[])
	 */
	@Override
	public void setPalette(ChartColor[] colors)
	{
		palette = colors;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPaletteEntry(int, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPaletteEntry(int index, ChartColor color)
	{
		try
		{
			palette[index] = color;
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#is3DModeEnabled()
	 */
	@Override
	public boolean is3DModeEnabled()
	{
		return displayIn3D;
	}

	/**
	 * Create default palette from preferences
	 */
	protected void createDefaultPalette()
	{
		palette = new ChartColor[MAX_CHART_ITEMS];
		for(int i = 0; i < MAX_CHART_ITEMS; i++)
		{
			palette[i] = ChartColor.createFromPreferences(preferenceStore, "Chart.Colors.Data." + i);
		}
	}
}
