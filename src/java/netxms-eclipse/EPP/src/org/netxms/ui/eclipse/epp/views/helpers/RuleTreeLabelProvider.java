/**
 * 
 */
package org.netxms.ui.eclipse.epp.views.helpers;

import org.eclipse.jface.viewers.LabelProvider;

/**
 * Label provider for rule tree
 *
 */
public class RuleTreeLabelProvider extends LabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		RuleTreeElement e = (RuleTreeElement)element;
		
		switch(e.getType())
		{
			case RuleTreeElement.POLICY:
				return "Policy";
			case RuleTreeElement.RULE:
				return "Rule " + Integer.toString(e.getRuleNumber()) + e.getCommentString();
			case RuleTreeElement.FILTER:
				return "Filter";
			case RuleTreeElement.ACTION:
				return "Action";
			case RuleTreeElement.SOURCE:
				return "Source";
			case RuleTreeElement.EVENT:
				return "Event";
			case RuleTreeElement.SEVERITY:
				return "Severity";
			case RuleTreeElement.SCRIPT:
				return "Script";
			case RuleTreeElement.ALARM:
				return "Alarm";
			case RuleTreeElement.ACTIONS:
				return "Actions";
			case RuleTreeElement.SITUATION:
				return "Situation";
			case RuleTreeElement.COMMENTS:
				return "Comments";
			default:
				return null;
		}
	}

}
