/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for "dial chart" dashboard element
 */
public class DialChartConfig extends AbstractChartConfig
{
	@Element(required = false)
	private boolean showTitle = true;
	
	@Element(required = false)
	private double minValue = 0.0;
	
	@Element(required = false)
	private double maxValue = 100.0;
	
	@Element(required = false)
	private double leftYellowZone = 0.0;
	
	@Element(required = false)
	private double leftRedZone = 0.0;
	
	@Element(required = false)
	private double rightYellowZone = 70.0;
	
	@Element(required = false)
	private double rightRedZone = 90.0;
	
	/**
	 * Create dial chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static DialChartConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(DialChartConfig.class, xml);
	}

	/**
	 * @return the maxValue
	 */
	public double getMaxValue()
	{
		return maxValue;
	}

	/**
	 * @param maxValue the maxValue to set
	 */
	public void setMaxValue(double maxValue)
	{
		this.maxValue = maxValue;
	}

	/**
	 * @return the yellowZone
	 */
	public double getRightYellowZone()
	{
		return rightYellowZone;
	}

	/**
	 * @param yellowZone the yellowZone to set
	 */
	public void setRightYellowZone(double yellowZone)
	{
		this.rightYellowZone = yellowZone;
	}

	/**
	 * @return the redZone
	 */
	public double getRightRedZone()
	{
		return rightRedZone;
	}

	/**
	 * @param redZone the redZone to set
	 */
	public void setRightRedZone(double redZone)
	{
		this.rightRedZone = redZone;
	}

	/**
	 * @return the showTitle
	 */
	public boolean isShowTitle()
	{
		return showTitle;
	}

	/**
	 * @param showTitle the showTitle to set
	 */
	public void setShowTitle(boolean showTitle)
	{
		this.showTitle = showTitle;
	}

	/**
	 * @return the minValue
	 */
	public double getMinValue()
	{
		return minValue;
	}

	/**
	 * @param minValue the minValue to set
	 */
	public void setMinValue(double minValue)
	{
		this.minValue = minValue;
	}

	/**
	 * @return the leftYellowZone
	 */
	public double getLeftYellowZone()
	{
		return leftYellowZone;
	}

	/**
	 * @param leftYellowZone the leftYellowZone to set
	 */
	public void setLeftYellowZone(double leftYellowZone)
	{
		this.leftYellowZone = leftYellowZone;
	}

	/**
	 * @return the leftRedZone
	 */
	public double getLeftRedZone()
	{
		return leftRedZone;
	}

	/**
	 * @param leftRedZone the leftRedZone to set
	 */
	public void setLeftRedZone(double leftRedZone)
	{
		this.leftRedZone = leftRedZone;
	}
}
