/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Event in log parser rule
 */
@Root(name="event", strict=false)
public class LogParserEvent
{
	@Attribute(name="params", required=false)
	private int parameterCount;
	
	@Text
	private String event;

	/**
	 * @return the parameterCount
	 */
	public int getParameterCount()
	{
		return parameterCount;
	}

	/**
	 * @param parameterCount the parameterCount to set
	 */
	public void setParameterCount(int parameterCount)
	{
		this.parameterCount = parameterCount;
	}

	/**
	 * @return the event
	 */
	public String getEvent()
	{
		return event;
	}

	/**
	 * @param event the event to set
	 */
	public void setEvent(String event)
	{
		this.event = event;
	}
}
