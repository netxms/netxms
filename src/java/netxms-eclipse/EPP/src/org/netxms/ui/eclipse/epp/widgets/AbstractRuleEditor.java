/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Abstract rule editor. Base class for all rule editing widgets.
 *
 */
public abstract class AbstractRuleEditor extends Composite
{
	protected EventProcessingPolicyRule rule;
	
	public AbstractRuleEditor(Composite parent, EventProcessingPolicyRule rule)
	{
		super(parent, SWT.NONE);
		this.rule = rule;
	}
}
