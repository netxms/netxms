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
	@Element
	private double maxValue = 100.0;
	
	@Element
	private double yellowZone = 70.0;
	
	@Element
	private double redZone = 90.0;
	
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
	public double getYellowZone()
	{
		return yellowZone;
	}

	/**
	 * @param yellowZone the yellowZone to set
	 */
	public void setYellowZone(double yellowZone)
	{
		this.yellowZone = yellowZone;
	}

	/**
	 * @return the redZone
	 */
	public double getRedZone()
	{
		return redZone;
	}

	/**
	 * @param redZone the redZone to set
	 */
	public void setRedZone(double redZone)
	{
		this.redZone = redZone;
	}
}
