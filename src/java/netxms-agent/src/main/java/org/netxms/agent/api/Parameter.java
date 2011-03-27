/**
 * 
 */
package org.netxms.agent.api;

/**
 * Agent's parameter
 */
public abstract class Parameter
{
	public static final int DT_INT    = 0;
	public static final int DT_UINT   = 1;
	public static final int DT_INT64  = 2;
	public static final int DT_UINT64 = 3;
	public static final int DT_STRING = 4;
	public static final int DT_FLOAT  = 5;
	public static final int DT_NULL   = 6;
	
	public static final String RC_NOT_SUPPORTED = "$not_supported$";
	public static final String RC_ERROR = "$error$";
	
	private String name;
	private String description;
	private int type;
	
	/**
	 * @param name
	 * @param description
	 * @param type
	 */
	public Parameter(String name, String description, int type)
	{
		this.name = name;
		this.description = description;
		this.type = type;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
	
	/**
	 * Parameter's handler.
	 * 
	 * @param parameter Actual parameter requested by server
	 * @return Parameter's value or one of the error indicators
	 */
	public abstract String handler(String parameter);
}
