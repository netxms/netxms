/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * Threshold definition for table DCI
 */
public class TableThreshold
{
	private static final String[] OPERATIONS = { "<", "<=", "==", ">=", ">", "!=", "like", "!like" };
	
	private long id;
	private int activationEvent;
	private int deactivationEvent;
	private List<List<TableCondition>> conditions;
	private long nextVarId;
	
	/**
	 * Create new empty threshold
	 */
	public TableThreshold()
	{
		id = 0;
		activationEvent = 0;
		deactivationEvent = 0;
		conditions = new ArrayList<List<TableCondition>>(0);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src
	 */
	public TableThreshold(TableThreshold src)
	{
		id = src.id;
		activationEvent = src.activationEvent;
		deactivationEvent = src.deactivationEvent;
		conditions = new ArrayList<List<TableCondition>>(src.conditions.size());
		for(List<TableCondition> sl : src.conditions)
		{
			List<TableCondition> dl = new ArrayList<TableCondition>(sl.size());
			for(TableCondition c : sl)
				dl.add(new TableCondition(c));
			conditions.add(dl);
		}
	}
	
	/**
	 * Create from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	protected TableThreshold(NXCPMessage msg, long baseId)
	{
		long varId = baseId;
		id = msg.getVariableAsInt64(varId++);
		activationEvent = msg.getVariableAsInteger(varId++);
		deactivationEvent = msg.getVariableAsInteger(varId++);
		
		int groupCount = msg.getVariableAsInteger(varId++);
		conditions = new ArrayList<List<TableCondition>>(groupCount);
		for(int i = 0; i < groupCount; i++)
		{
			int condCount = msg.getVariableAsInteger(varId++);
			List<TableCondition> list = new ArrayList<TableCondition>(condCount);
			for(int j = 0; j < condCount; j++)
			{
				list.add(new TableCondition(msg, varId));
				varId += 3;
			}
			conditions.add(list);
		}
		nextVarId = varId;
	}
	
	/**
	 * Fill NXCP message with threshold data
	 * 
	 * @param msg
	 * @param baseId
	 * @return
	 */
	protected long fillMessage(NXCPMessage msg, long baseId)
	{
		long varId = baseId;
		
		msg.setVariableInt32(varId++, (int)id);
		msg.setVariableInt32(varId++, activationEvent);
		msg.setVariableInt32(varId++, deactivationEvent);
		msg.setVariableInt32(varId++, conditions.size());
		for(List<TableCondition> l : conditions)
		{
			msg.setVariableInt32(varId++, l.size());
			for(TableCondition c : l)
			{
				msg.setVariable(varId++, c.getColumn());
				msg.setVariableInt16(varId++, c.getOperation());
				msg.setVariable(varId++, c.getValue());
			}
		}
		
		return varId;
	}
	
	/**
	 * Get threshold condition as text
	 * 
	 * @return
	 */
	public String getConditionAsText()
	{
		StringBuilder sb = new StringBuilder();
		for(List<TableCondition> group : conditions)
		{
			if (sb.length() > 0)
				sb.append(" OR ");
			if (conditions.size() > 1)
				sb.append('(');
			for(TableCondition cond : group)
			{
				if (cond != group.get(0))
					sb.append(" AND ");
				sb.append(cond.getColumn());
				sb.append(' ');
				try
				{
					sb.append(OPERATIONS[cond.getOperation()]);
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					sb.append('?');
				}
				sb.append(' ');
				sb.append(cond.getValue());
			}
			if (conditions.size() > 1)
				sb.append(')');
		}
		return sb.toString();
	}

	/**
	 * @return the nextVarId
	 */
	public long getNextVarId()
	{
		return nextVarId;
	}

	/**
	 * @return the activationEvent
	 */
	public int getActivationEvent()
	{
		return activationEvent;
	}

	/**
	 * @param activationEvent the activationEvent to set
	 */
	public void setActivationEvent(int activationEvent)
	{
		this.activationEvent = activationEvent;
	}

	/**
	 * @return the deactivationEvent
	 */
	public int getDeactivationEvent()
	{
		return deactivationEvent;
	}

	/**
	 * @param deactivationEvent the deactivationEvent to set
	 */
	public void setDeactivationEvent(int deactivationEvent)
	{
		this.deactivationEvent = deactivationEvent;
	}

	/**
	 * @return the conditions
	 */
	public List<List<TableCondition>> getConditions()
	{
		return conditions;
	}

	/**
	 * @param conditions the conditions to set
	 */
	public void setConditions(List<List<TableCondition>> conditions)
	{
		this.conditions = conditions;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}
}
