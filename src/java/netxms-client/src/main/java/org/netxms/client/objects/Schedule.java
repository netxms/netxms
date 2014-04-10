package org.netxms.client.objects;

import org.netxms.base.NXCPMessage;

public class Schedule 
{
	public static final int TYPE_WEEKLY = 1;
	public static final int TYPE_MONTHLY = 2;
	public static final int TYPE_YEARLY = 3;
	
	private int id;
	private int instId;
	private int type;
	private String name;
	private char[] dayMap;
	

	public Schedule(String name, int type)
	{
		id = 0;
		instId = 0;
		this.type = type;
		this.name = name;
		
		switch (type)
		{
			case TYPE_WEEKLY:
				dayMap = new char[7];
				for (int i = 0; i < 7; i++)
				{
					if (i == 5 || i == 6)
						dayMap[i] = '0';
					else
						dayMap[i] = '1';
				}
				break;
			case TYPE_MONTHLY:
				dayMap = new char[31];
				for (int i = 0; i < 31; i++)
						dayMap[i] = '1';
				break;
			case TYPE_YEARLY:
				dayMap = new char[366];
				for (int i = 0; i < 366; i++)
					dayMap[i] = '1';
				break;
			default:
				break;
		}
	}
	
	public Schedule(NXCPMessage message, long varId)
	{
		id = message.getVariableAsInteger(varId++);
		instId = message.getVariableAsInteger(varId++);
		type = message.getVariableAsInteger(varId++);
		name = message.getVariableAsString(varId++);
		dayMap = message.getVariableAsString(varId++).toCharArray();
	}

	/**
	 * @return the type
	 */
	public int getType() 
	{
		return type;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type) 
	{
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
	 * @param name the name to set
	 */
	public void setName(String name) 
	{
		this.name = name;
	}

	/**
	 * @return the dayMap
	 */
	public char[] getDayMap() 
	{
		return dayMap;
	}

	/**
	 * @param dayMap the dayMap to set
	 */
	public void setDayMap(char[] dayMap) 
	{
		this.dayMap = dayMap;
	}

	/**
	 * @return the id
	 */
	public int getId() 
	{
		return id;
	}
	
	/**
	 * @return the instId
	 */
	public int getInstId() 
	{
		return instId;
	}

	/**
	 * @param instId the instId to set
	 */
	public void setInstId(int instId) 
	{
		this.instId = instId;
	}

}
