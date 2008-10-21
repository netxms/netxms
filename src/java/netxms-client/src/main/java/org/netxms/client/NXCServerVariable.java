/**
 * 
 */
package org.netxms.client;

/**
 * @author Victor
 *
 */
public final class NXCServerVariable
{
	private String name;
	private String value;
	private boolean isServerRestartNeeded;

	
	/**
	 * Default constructor for NXCServerVariable.
	 * 
	 * @param name Variable's name
	 * @param value Variable's value
	 * @param isServerRestartNeeded Server restart flag (server has to be restarted after variable change if this flag is set)
	 */
	public NXCServerVariable(String name, String value, boolean isServerRestartNeeded)
	{
		this.name = name;
		this.value = value;
		this.isServerRestartNeeded = isServerRestartNeeded;
	}


	/**
	 * @return Varaible's name
	 */
	public String getName()
	{
		return name;
	}


	/**
	 * @return Variable's value
	 */
	public String getValue()
	{
		return value;
	}


	/**
	 * @return Server restart flag
	 */
	public boolean isServerRestartNeeded()
	{
		return isServerRestartNeeded;
	}
}
