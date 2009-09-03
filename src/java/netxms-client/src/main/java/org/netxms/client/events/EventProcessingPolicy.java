/**
 * 
 */
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.List;


/**
 * This class represents NetXMS event processing policy.
 * 
 * @author Victor
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
	 * @return the rules
	 */
	public List<EventProcessingPolicyRule> getRules()
	{
		return rules;
	}
}
