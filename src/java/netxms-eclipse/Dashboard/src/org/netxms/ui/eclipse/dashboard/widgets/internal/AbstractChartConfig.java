package org.netxms.ui.eclipse.dashboard.widgets.internal;

import org.netxms.client.datacollection.GraphSettings;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;

public abstract class AbstractChartConfig extends DashboardElementConfig
{

	@ElementArray(required = true)
	private DashboardDciInfo[] dciList = new DashboardDciInfo[0];
	@Element(required = false)
	private String title = "";
	@Element(required = false)
	private int legendPosition = GraphSettings.POSITION_RIGHT;
	@Element(required = false)
	private boolean showLegend = true;

	public AbstractChartConfig()
	{
		super();
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @return the dciList
	 */
	public DashboardDciInfo[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(DashboardDciInfo[] dciList)
	{
		this.dciList = dciList;
	}

	/**
	 * @return the legendPosition
	 */
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/**
	 * @param legendPosition the legendPosition to set
	 */
	public void setLegendPosition(int legendPosition)
	{
		this.legendPosition = legendPosition;
	}

	/**
	 * @return the showLegend
	 */
	public boolean isShowLegend()
	{
		return showLegend;
	}

	/**
	 * @param showLegend the showLegend to set
	 */
	public void setShowLegend(boolean showLegend)
	{
		this.showLegend = showLegend;
	}

}