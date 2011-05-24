/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.core.runtime.IAdapterManager;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.dashboard.api.CustomDashboardElement;
import org.netxms.ui.eclipse.dashboard.api.DashboardElementCreationData;
import org.netxms.ui.eclipse.dashboard.widgets.internal.CustomWidgetConfig;

/**
 * Dashboard element containing custom widget
 */
public class CustomWidgetElement extends ElementWidget
{
	private CustomWidgetConfig config; 
	private CustomDashboardElement control;
	
	/**
	 * @param parent
	 * @param data
	 */
	public CustomWidgetElement(Composite parent, String data)
	{
		super(parent, SWT.NONE, data);
		
		try
		{
			config = CustomWidgetConfig.createFromXml(data);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new CustomWidgetConfig();
		}
		
		FillLayout layout = new FillLayout();
		setLayout(layout);

		IAdapterManager adapterManager = Platform.getAdapterManager();
		control = (CustomDashboardElement)adapterManager.loadAdapter(new DashboardElementCreationData(this, data), config.getClassName());
	}
}
