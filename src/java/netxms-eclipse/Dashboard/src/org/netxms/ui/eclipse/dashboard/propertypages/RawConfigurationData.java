/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Layout" page for dashboard element
 */
public class RawConfigurationData extends PropertyPage
{
	private LabeledText text; 
	private DashboardElement element;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		element = (DashboardElement)getElement().getAdapter(DashboardElement.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		text = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
		text.setLabel("Configuration XML");
		text.setText(element.getData());
		GridData gd = new GridData();
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
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		element.setData(text.getText());
		return true;
	}
}
