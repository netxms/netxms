/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;

/**
 * @author victor
 *
 */
public class CommentsEditor extends AbstractRuleEditor
{
	private Text text;
	
	public CommentsEditor(Composite parent, final EventProcessingPolicyRule rule, EventProcessingPolicyEditor editor)
	{
		super(parent, rule, editor);
		setLayout(new FillLayout());
		text = new Text(this, SWT.MULTI);
		text.setText(rule.getComments());
	}
}
