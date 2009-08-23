/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class AbstractSelector extends Composite
{
	private Label label;
	private Text text;
	private Button button;
	
	/**
	 * Create abstract selector.
	 * 
	 * @param parent
	 * @param style
	 */
	public AbstractSelector(Composite parent, int style)
	{
		super(parent, style);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		setLayout(layout);
		
		label = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		label.setLayoutData(gd);
		
		text = new Text(this, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		
		button = new Button(this, SWT.PUSH);
		gd = new GridData();
		gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
		button.setLayoutData(gd);
		button.setText("...");
		button.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectionButtonHandler();
			}
		});
	}
	
	/**
	 * Handler for selection button. This method intended to be overriden by subclasses.
	 * Default implementation does nothing.
	 */
	protected void selectionButtonHandler()
	{
	}

	/**
	 * Set selector's label
	 * 
	 * @param newLabel New label
	 */
	public void setLabel(final String newLabel)
	{
		label.setText(newLabel);
	}
	
	/**
	 * Get selector's label
	 * 
	 * @return Current selector's label
	 */
	public String getLabel()
	{
		return label.getText();
	}
	
	/**
	 * Set selector's text
	 * 
	 * @param newText
	 */
	public void setText(final String newText)
	{
		text.setText(newText);
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	public String getText()
	{
		return text.getText();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		text.setEnabled(enabled);
		button.setEnabled(enabled);
		super.setEnabled(enabled);
	}
}
