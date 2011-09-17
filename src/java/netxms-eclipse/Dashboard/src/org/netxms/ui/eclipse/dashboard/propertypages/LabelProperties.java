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
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Label configuration page
 */
public class LabelProperties extends PropertyPage
{
	private LabeledText title; 
	private DashboardElement element;
	private LabelConfig config;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		element = (DashboardElement)getElement().getAdapter(DashboardElement.class);
		try
		{
			config = LabelConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new LabelConfig();
		}
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		title = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
		title.setLabel("Title");
		title.setText(config.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		return dialogArea;
	}
}
