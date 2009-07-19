/**
 * 
 */
package org.netxms.client;

import java.util.ArrayList;
import java.util.List;

/**
 * This class represents NetXMS event processing policy.
 * 
 * @author Victor
 */
public class NXCEventProcessingPolicy
{
	private List<NXCEventProcessingPolicyRule> rules;

	/**
	 * Create new policy object.
	 * 
	 * @param numRules Expected number of rules
	 */
	protected NXCEventProcessingPolicy(int numRules)
	{
		rules = new ArrayList<NXCEventProcessingPolicyRule>(numRules);
	}
	
	/**
	 * Add new rule.
	 * 
	 * @param rule Rule to add
	 */
	protected void addRule(NXCEventProcessingPolicyRule rule)
	{
		rules.add(rule);
	}

	/**
	 * @return the rules
	 */
	public List<NXCEventProcessingPolicyRule> getRules()
	{
		return rules;
	}
}
