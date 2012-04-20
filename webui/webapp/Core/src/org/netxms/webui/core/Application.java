package org.netxms.webui.core;

import org.eclipse.equinox.app.IApplication;
import org.eclipse.equinox.app.IApplicationContext;
import org.eclipse.rwt.RWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.netxms.api.client.Session;
import org.netxms.ui.eclipse.console.resources.DataCollectionDisplayInfo;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * This class controls all aspects of the application's execution and is
 * contributed through the plugin.xml.
 */
public class Application implements IApplication
{
	/* (non-Javadoc)
	 * @see org.eclipse.equinox.app.IApplication#start(org.eclipse.equinox.app.IApplicationContext)
	 */
	public Object start(IApplicationContext context) throws Exception
	{
		Display display = PlatformUI.createDisplay();
		RWT.getSessionStore().getHttpSession().setMaxInactiveInterval(120);
		display.disposeExec(new Runnable() {
			public void run()
			{
				Session session = ConsoleSharedData.getSession();
				if (session != null)
					session.disconnect();
			}
		});

		SharedIcons.init(display);
		SharedColors.init(display);
		StatusDisplayInfo.init(display);
		DataCollectionDisplayInfo.init();
		WorkbenchAdvisor advisor = new ApplicationWorkbenchAdvisor();
		return PlatformUI.createAndRunWorkbench(display, advisor);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.app.IApplication#stop()
	 */
	public void stop()
	{
		// Do nothing
	}
}
