/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class LabeledText extends Composite
{
	private Label label;
	private Text text;

	/**
	 * @param parent
	 * @param style
	 */
	public LabeledText(Composite parent, int style)
	{
		super(parent, style);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);
		
		label = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		label.setLayoutData(gd);
		
		text = new Text(this, SWT.SINGLE | SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
	}
	
	/**
	 * Set label
	 * 
	 * @param newLabel New label
	 */
	public void setLabel(final String newLabel)
	{
		label.setText(newLabel);
	}
	
	/**
	 * Get label
	 * 
	 * @return Current label
	 */
	public String getLabel()
	{
		return label.getText();
	}
	
	/**
	 * Set text
	 * 
	 * @param newText
	 */
	public void setText(final String newText)
	{
		text.setText(newText);
	}

	/**
	 * Get text
	 * 
	 * @return Text
	 */
	public String getText()
	{
		return text.getText();
	}
	
	/**
	 * Get text control
	 * 
	 * @return text control
	 */
	public Text getTextControl()
	{
		return text;
	}
}
