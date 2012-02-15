/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Context in log parser rule
 */
@Root(name="context", strict=false)
public class LogParserContext
{
	@Attribute
	private String action = "set";
	
	@Attribute(required=false)
	private String reset = "auto";
	
	@Text
	private String data;

	/**
	 * @return the action
	 */
	public String getAction()
	{
		return action;
	}

	/**
	 * @param action the action to set
	 */
	public void setAction(String action)
	{
		this.action = action;
	}

	/**
	 * @return the reset
	 */
	public String getReset()
	{
		return reset;
	}

	/**
	 * @param reset the reset to set
	 */
	public void setReset(String reset)
	{
		this.reset = reset;
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}
}
