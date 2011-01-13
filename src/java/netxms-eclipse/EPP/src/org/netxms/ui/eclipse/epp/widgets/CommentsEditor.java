/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * @author victor
 *
 */
public class CommentsEditor extends AbstractRuleEditor
{
	private Text text;
	
	public CommentsEditor(Composite parent, EventProcessingPolicyRule rule)
	{
		super(parent, rule);
		setLayout(new FillLayout());
		text = new Text(this, SWT.MULTI);
		text.setText(rule.getComments());
	}

}
