/**
 * 
 */
package org.netxms.ui.eclipse.charts;

import java.io.StringWriter;
import java.io.Writer;
import org.eclipse.swt.graphics.RGB;
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
	public static final int LINE_GRAPH = 0;
	public static final int AREA_GRAPH = 1;
	
	@Element(required=false)
	private boolean enabled = false;
	
	@Element(required=false)
	private int type = LINE_GRAPH;
	
	@Element(required=false)
	private int color = 0x00C000;
	
	@Element(required=false)
	private String title = null;

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
	public RGB getColor()
	{
		return new RGB(color & 0xFF, (color >> 8) & 0xFF, color >> 16);
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
	public void setColor(final RGB color)
	{
		this.color = (color.blue << 16) | (color.green << 8) | color.red;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}
}
