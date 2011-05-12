/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for dashboard element
 */
@Root(name="dci")
public class DashboardDciInfo
{
	@Attribute
	public long nodeId = 0;
	
	@Attribute
	public long dciId = 0;
	
	@Element(required=false)
	public String color = "0xC00000";

	@Element(required=false)
	public String name = "";

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
	 * @param value
	 */
	public void setColor(int value)
	{
		color = "0x" + Integer.toHexString(value);
	}
	
	public String getName()
	{
		return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(dciId) + "]");
	}
}
