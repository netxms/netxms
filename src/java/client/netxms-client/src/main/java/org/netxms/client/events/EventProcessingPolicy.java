/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.List;

/**
 * This class represents NetXMS event processing policy.
 */
public class EventProcessingPolicy
{
	private List<EventProcessingPolicyRule> rules;

	/**
	 * Create new policy object.
	 * 
	 * @param numRules Expected number of rules
	 */
	public EventProcessingPolicy(int numRules)
	{
		rules = new ArrayList<EventProcessingPolicyRule>(numRules);
	}
	
	/**
	 * Add new rule.
	 * 
	 * @param rule Rule to add
	 */
	public void addRule(EventProcessingPolicyRule rule)
	{
		rules.add(rule);
	}
	
	/**
	 * Insert rule before rule at given position
	 * 
	 * @param rule rule to insert
	 * @param index position to insert at
	 */
	public void insertRule(EventProcessingPolicyRule rule, int index)
	{
		rules.add(index, rule);
	}

	/**
	 * Delete rule.
	 * 
	 * @param index zero-based index
	 */
	public void deleteRule(int index)
	{
		try
		{
			rules.remove(index);
		}
		catch(IndexOutOfBoundsException e)
		{
		}
	}

	/**
	 * @return the rules
	 */
	public List<EventProcessingPolicyRule> getRules()
	{
		return rules;
	}
}
