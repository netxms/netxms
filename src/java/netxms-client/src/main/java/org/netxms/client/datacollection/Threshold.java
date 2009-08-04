/**
 * 
 */
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Represents data collection item's threshold
 * 
 * @author Victor Kirhenshtein
 *
 */
public class Threshold
{
	public static final int F_LAST      = 0;
	public static final int F_AVERAGE   = 1;
	public static final int F_DEVIATION = 2;
	public static final int F_DIFF      = 3;
	public static final int F_ERROR     = 4;
	
	public static final int OP_LE       = 0;
	public static final int OP_LE_EQ    = 1;
	public static final int OP_EQ       = 2;
	public static final int OP_GT_EQ    = 3;
	public static final int OP_GT       = 4;
	public static final int OP_NE       = 5;
	public static final int OP_LIKE     = 6;
	public static final int OP_NOTLIKE  = 7;
	
	private long id;
	private int fireEvent;
	private int rearmEvent;
	private int arg1;
	private int arg2;
	private int function;
	private int operation;
	private int repeatInterval;
	private String value;
	
	/**
	 * Create DCI threshold object from NXCP message
	 * @param msg NXCP message
	 * @param baseId Base variable ID for this threshold in message
	 */
	protected Threshold(final NXCPMessage msg, final long baseId)
	{
		long varId = baseId;
		
		id = msg.getVariableAsInt64(varId++);
		fireEvent = msg.getVariableAsInteger(varId++);
		rearmEvent = msg.getVariableAsInteger(varId++);
		function = msg.getVariableAsInteger(varId++);
		operation = msg.getVariableAsInteger(varId++);
		arg1 = msg.getVariableAsInteger(varId++);
		arg2 = msg.getVariableAsInteger(varId++);
		repeatInterval = msg.getVariableAsInteger(varId++);
		value = msg.getVariableAsString(varId);
	}
	
	/**
	 * Create new threshold object
	 */
	public Threshold()
	{
		id = 0;
		fireEvent = 17;
		rearmEvent = 18;
		arg1 = 0;
		arg2 = 0;
		function = F_LAST;
		operation = OP_LE;
		repeatInterval = -1;
		value = "0";
	}

	/**
	 * @return the fireEvent
	 */
	public int getFireEvent()
	{
		return fireEvent;
	}

	/**
	 * @param fireEvent the fireEvent to set
	 */
	public void setFireEvent(int fireEvent)
	{
		this.fireEvent = fireEvent;
	}

	/**
	 * @return the rearmEvent
	 */
	public int getRearmEvent()
	{
		return rearmEvent;
	}

	/**
	 * @param rearmEvent the rearmEvent to set
	 */
	public void setRearmEvent(int rearmEvent)
	{
		this.rearmEvent = rearmEvent;
	}

	/**
	 * @return the arg1
	 */
	public int getArg1()
	{
		return arg1;
	}

	/**
	 * @param arg1 the arg1 to set
	 */
	public void setArg1(int arg1)
	{
		this.arg1 = arg1;
	}

	/**
	 * @return the arg2
	 */
	public int getArg2()
	{
		return arg2;
	}

	/**
	 * @param arg2 the arg2 to set
	 */
	public void setArg2(int arg2)
	{
		this.arg2 = arg2;
	}

	/**
	 * @return the function
	 */
	public int getFunction()
	{
		return function;
	}

	/**
	 * @param function the function to set
	 */
	public void setFunction(int function)
	{
		this.function = function;
	}

	/**
	 * @return the operation
	 */
	public int getOperation()
	{
		return operation;
	}

	/**
	 * @param operation the operation to set
	 */
	public void setOperation(int operation)
	{
		this.operation = operation;
	}

	/**
	 * @return the repeatInterval
	 */
	public int getRepeatInterval()
	{
		return repeatInterval;
	}

	/**
	 * @param repeatInterval the repeatInterval to set
	 */
	public void setRepeatInterval(int repeatInterval)
	{
		this.repeatInterval = repeatInterval;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @param value the value to set
	 */
	public void setValue(String value)
	{
		this.value = value;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}
}
