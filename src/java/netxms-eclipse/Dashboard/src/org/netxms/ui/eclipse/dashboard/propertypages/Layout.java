/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Layout" page for dashboard element
 */
public class Layout extends PropertyPage
{
	private Combo comboHorizontalAlign;
	private Combo comboVerticalAlign;
	private Spinner spinnerHorizontalSpan;
	private Spinner spinnerVerticalSpan;
	private DashboardElement element;
	private DashboardElementLayout elementLayout;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		element = (DashboardElement)getElement().getAdapter(DashboardElement.class);
		try
		{
			elementLayout = DashboardElementLayout.createFromXml(element.getLayout());
		}
		catch(Exception e)
		{
			elementLayout = new DashboardElementLayout();
		}
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		comboHorizontalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				"Horizontal alignment", WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboHorizontalAlign.add("FILL");
		comboHorizontalAlign.add("CENTER");
		comboHorizontalAlign.add("LEFT");
		comboHorizontalAlign.add("RIGHT");
		comboHorizontalAlign.select(elementLayout.horizontalAlignment);
		
		comboVerticalAlign = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, 
				"Vertical alignment", WidgetHelper.DEFAULT_LAYOUT_DATA);
		comboVerticalAlign.add("FILL");
		comboVerticalAlign.add("CENTER");
		comboVerticalAlign.add("TOP");
		comboVerticalAlign.add("BOTTOM");
		comboVerticalAlign.select(elementLayout.vertcalAlignment);
		
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
		spinnerHorizontalSpan.setSelection(elementLayout.horizontalSpan);

		spinnerVerticalSpan = (Spinner)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, 
				"Vertical span", WidgetHelper.DEFAULT_LAYOUT_DATA);
		spinnerVerticalSpan.setSelection(elementLayout.verticalSpan);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		elementLayout.horizontalAlignment = comboHorizontalAlign.getSelectionIndex();
		elementLayout.vertcalAlignment = comboVerticalAlign.getSelectionIndex();
		elementLayout.horizontalSpan = spinnerHorizontalSpan.getSelection();
		elementLayout.verticalSpan = spinnerVerticalSpan.getSelection();
		
		try
		{
			element.setLayout(elementLayout.createXml());
		}
		catch(Exception e)
		{
			MessageDialog.openError(getShell(), "Error", "Cannot update dashboard element: data serialization error");
			return false;
		}
		
		return true;
	}
}
