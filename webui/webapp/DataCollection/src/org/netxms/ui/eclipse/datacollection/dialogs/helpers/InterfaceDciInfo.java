/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs.helpers;

/**
 * Information for creating interface DCI
 */
public class InterfaceDciInfo
{
	public boolean enabled;
	public boolean delta;
	public String description;
	
	/**
	 * @param enabled
	 * @param delta
	 * @param description
	 */
	public InterfaceDciInfo(boolean enabled, boolean delta, String description)
	{
		super();
		this.enabled = enabled;
		this.delta = delta;
		this.description = description;
	}
}
