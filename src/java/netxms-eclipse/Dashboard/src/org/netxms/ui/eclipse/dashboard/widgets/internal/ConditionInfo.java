package org.netxms.ui.eclipse.dashboard.widgets.internal;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

@Root(name = "condition")
public class ConditionInfo
{
	private static final String UNSET_COLOR = "UNSET";

	@Attribute
	public long id = 0;

	@Element(required = false)
	public String color = UNSET_COLOR;

	@Element(required = false)
	public String name = "";

	private boolean active = false;

	/**
	 * @return the color
	 */
	public int getColorAsInt()
	{
		if (color.equals(UNSET_COLOR))
			return -1;
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

	/**
	 * Get Condition name. Always returns non-empty string.
	 * 
	 * @return
	 */
	public String getName()
	{
		return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(id) + "]");
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	public void setActive(boolean active)
	{
		this.active = active;
	}

	/**
	 * @return the active
	 */
	public boolean isActive()
	{
		return active;
	}

}
