/**
 * 
 */
package org.netxms.ui.eclipse.epp.views.helpers;

import java.util.ArrayList;
import java.util.List;

import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Element of rule tree
 *
 */
public class RuleTreeElement
{
	public static final int POLICY = 0;
	public static final int RULE = 10;
	public static final int FILTER = 11;
	public static final int ACTION = 12;
	public static final int COMMENTS = 13;
	public static final int SOURCE = 20;
	public static final int EVENT = 21;
	public static final int SEVERITY = 22;
	public static final int SCRIPT = 23;
	public static final int ALARM = 30;
	public static final int ACTIONS = 31;
	public static final int SITUATION = 32;
	
	private int type;
	private EventProcessingPolicyRule rule;
	private int ruleNumber;
	private RuleTreeElement parent;
	private List<RuleTreeElement> childs = new ArrayList<RuleTreeElement>();
	
	/**
	 * Create rule tree element.
	 * 
	 * @param rule
	 * @param type
	 */
	public RuleTreeElement(EventProcessingPolicyRule rule, int ruleNumber, int type, RuleTreeElement parent)
	{
		this.rule = rule;
		this.ruleNumber = ruleNumber;
		this.type = type;
		this.parent = parent;
		if (parent != null)
			parent.addChild(this);
	}
	
	/**
	 * Add child element
	 * @param e
	 */
	private void addChild(RuleTreeElement e)
	{
		childs.add(e);
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the rule
	 */
	public EventProcessingPolicyRule getRule()
	{
		return rule;
	}

	/**
	 * @return the parent
	 */
	public RuleTreeElement getParent()
	{
		return parent;
	}

	/**
	 * @return the childs
	 */
	public RuleTreeElement[] getChilds()
	{
		return childs.toArray(new RuleTreeElement[childs.size()]);
	}
	
	/**
	 * Check if element ahs children
	 * @return
	 */
	public boolean hasChildren()
	{
		return childs.size() > 0;
	}

	/**
	 * @return the ruleNumber
	 */
	public int getRuleNumber()
	{
		return ruleNumber;
	}

	/**
	 * Return rule comments in ( ) or empty string if rule has no comments
	 * @return
	 */
	public String getCommentString()
	{
		if (rule.getComments().isEmpty())
			return "";
		
		return " (" + rule.getComments() + ")";
	}
}
