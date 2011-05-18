/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * @author Victor
 *
 */
public class NetworkMapElement extends ElementWidget
{
	private Dashboard object;
	private DashboardControl dashboard;
	private EmbeddedDashboardConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public NetworkMapElement(Composite parent, String data)
	{
		super(parent, data);

		try
		{
			config = EmbeddedDashboardConfig.createFromXml(data);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new EmbeddedDashboardConfig();
		}
		
		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		object = (Dashboard)session.findObjectById(config.getObjectId(), Dashboard.class);
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		setLayout(layout);
		
		if (object != null)
		{
			dashboard = new DashboardControl(this, SWT.NONE, object);
		}
	}

}
