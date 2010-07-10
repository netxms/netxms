/**
 * 
 */
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Log filter builder control
 *
 */
public class FilterBuilder extends Composite
{
	private Composite expression;
	private Button addButton;
	private Button deleteButton;
	private Button applyButton;
	private Button clearButton;

	/**
	 * @param parent
	 * @param style
	 */
	public FilterBuilder(Composite parent, int style)
	{
		super(parent, style);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		setLayout(layout);
		
		expression = new Composite(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		expression.setLayoutData(gd);

		Composite editButtons = new Composite(this, SWT.NONE);
		RowLayout btnLayout = new RowLayout();
		btnLayout.type = SWT.VERTICAL;
		btnLayout.fill = true;
		editButtons.setLayout(btnLayout);
		
		Composite ctrlButtons = new Composite(this, SWT.NONE);
		btnLayout = new RowLayout();
		btnLayout.type = SWT.HORIZONTAL;
		btnLayout.fill = true;
		ctrlButtons.setLayout(btnLayout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.horizontalSpan = 2;
		ctrlButtons.setLayoutData(gd);
		
		applyButton = new Button(ctrlButtons, SWT.PUSH);
		applyButton.setText("&Apply");
		RowData rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		applyButton.setLayoutData(rd);
		
		clearButton = new Button(ctrlButtons, SWT.PUSH);
		clearButton.setText("&Clear");
		rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		clearButton.setLayoutData(rd);
	}
}
