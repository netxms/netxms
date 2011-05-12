/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author victor
 *
 */
public class EditDashboardElementDlg extends Dialog
{
	private Combo comboHorizontalAlign;
	private Combo comboVerticalAlign;
	private Spinner spinnerHorizontalSpan;
	private Spinner spinnerVerticalSpan;
	private Text text;
	private DashboardElement element;
	
	public EditDashboardElementDlg(Shell parentShell, DashboardElement element)
	{
		super(parentShell);
		this.element = element;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		comboHorizontalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				"Horizontal alignment", WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboHorizontalAlign.add("FILL");
		comboHorizontalAlign.add("CENTER");
		comboHorizontalAlign.add("LEFT");
		comboHorizontalAlign.add("RIGHT");
		comboHorizontalAlign.select(element.getHorizontalAlignment());
		
		comboVerticalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				"Vertical alignment", WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboVerticalAlign.add("FILL");
		comboVerticalAlign.add("CENTER");
		comboVerticalAlign.add("TOP");
		comboVerticalAlign.add("BOTTOM");
		comboVerticalAlign.select(element.getVerticalAlignment());
		
		final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				Spinner spinner = new Spinner(parent, style);
				spinner.setMinimum(1);
				spinner.setMaximum(8);
				return spinner;
			}
		};
		
		spinnerHorizontalSpan = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				"Horizontal span", WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerHorizontalSpan.setSelection(element.getHorizontalSpan());

		spinnerVerticalSpan = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				"Vertical span", WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerVerticalSpan.setSelection(element.getVerticalSpan());

		text = new Text(dialogArea, SWT.BORDER | SWT.MULTI);
		text.setText(element.getData());
		GridData gd = new GridData();
		gd.horizontalSpan = 2;
		gd.verticalAlignment = SWT.FILL;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 400;
		gd.heightHint = 300;
		text.setLayoutData(gd);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		element.setHorizontalAlignment(comboHorizontalAlign.getSelectionIndex());
		element.setVerticalAlignment(comboVerticalAlign.getSelectionIndex());
		element.setHorizontalSpan(spinnerHorizontalSpan.getSelection());
		element.setVerticalSpan(spinnerVerticalSpan.getSelection());
		element.setData(text.getText());
		super.okPressed();
	}
}
