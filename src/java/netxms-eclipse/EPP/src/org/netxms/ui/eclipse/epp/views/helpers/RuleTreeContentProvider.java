/**
 * 
 */
package org.netxms.ui.eclipse.epp.views.helpers;

import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Content provider for rule tree
 *
 */
public class RuleTreeContentProvider implements ITreeContentProvider
{
	private RuleTreeElement root = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if (newInput instanceof EventProcessingPolicy)
		{
			root = new RuleTreeElement(null, 0, RuleTreeElement.POLICY, null);
			int ruleNumber = 0;
			for(EventProcessingPolicyRule rule : ((EventProcessingPolicy)newInput).getRules())
			{
				ruleNumber++;
				RuleTreeElement element = new RuleTreeElement(rule, ruleNumber, RuleTreeElement.RULE, root);
				
				RuleTreeElement filter = new RuleTreeElement(rule, ruleNumber, RuleTreeElement.FILTER, element);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.SOURCE, filter);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.EVENT, filter);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.SEVERITY, filter);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.SCRIPT, filter);
				
				RuleTreeElement action = new RuleTreeElement(rule, ruleNumber, RuleTreeElement.ACTION, element);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.ALARM, action);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.ACTIONS, action);
				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.SITUATION, action);

				new RuleTreeElement(rule, ruleNumber, RuleTreeElement.COMMENTS, element);
			}
		}
		else
		{
			root = null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (root == null)
			return new Object[0];
		return new Object[] { root };
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		return ((RuleTreeElement)parentElement).getChilds();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		return ((RuleTreeElement)element).getParent();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((RuleTreeElement)element).hasChildren();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
}
