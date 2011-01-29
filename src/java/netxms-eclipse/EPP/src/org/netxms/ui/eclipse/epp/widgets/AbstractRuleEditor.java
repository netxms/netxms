/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;

/**
 * Abstract rule editor. Base class for all rule editing widgets.
 *
 */
public abstract class AbstractRuleEditor extends Composite
{
	protected static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	
	protected EventProcessingPolicyRule rule;
	protected EventProcessingPolicyEditor editor;
	
	public AbstractRuleEditor(Composite parent, final EventProcessingPolicyRule rule, EventProcessingPolicyEditor editor)
	{
		super(parent, SWT.NONE);
		this.rule = rule;
		this.editor = editor;
		setBackground(BACKGROUND_COLOR);
	}
}
