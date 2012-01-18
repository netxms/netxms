/**
 * 
 */
package org.netxms.ui.eclipse.perfview;

import java.io.StringWriter;
import java.io.Writer;

import org.netxms.client.datacollection.GraphItemStyle;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Settings for performance tab graph
 *
 */
@Root(name="config")
public class PerfTabGraphSettings
{
	@Element(required=false)
	private boolean enabled = false;
	
	@Element(required=false)
	private int type = GraphItemStyle.LINE;
	
	@Element(required=false)
	private String color = "0x00C000";
	
	@Element(required=false)
	private String title = "";
	
	@Element(required=false)
	private boolean showThresholds = false;

	/**
	 * Create performance tab graph settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static PerfTabGraphSettings createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(PerfTabGraphSettings.class, xml);
	}
	
	/**
	 * Create XML document from object
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/**
	 * @return the enabled
	 */
	public boolean isEnabled()
	{
		return enabled;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the color
	 */
	public String getColor()
	{
		return color;
	}

	/**
	 * @return the color
	 */
	public int getColorAsInt()
	{
		if (color.startsWith("0x"))
			return Integer.parseInt(color.substring(2), 16);
		return Integer.parseInt(color, 10);
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param enabled the enabled to set
	 */
	public void setEnabled(boolean enabled)
	{
		this.enabled = enabled;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(final String color)
	{
		this.color = color;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(final int color)
	{
		this.color = Integer.toString(color);
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @return the showThresholds
	 */
	public boolean isShowThresholds()
	{
		return showThresholds;
	}

	/**
	 * @param showThresholds the showThresholds to set
	 */
	public void setShowThresholds(boolean showThresholds)
	{
		this.showThresholds = showThresholds;
	}
}
