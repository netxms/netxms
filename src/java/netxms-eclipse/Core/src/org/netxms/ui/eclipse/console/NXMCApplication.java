package org.netxms.ui.eclipse.console;

import org.eclipse.equinox.app.IApplication;
import org.eclipse.equinox.app.IApplicationContext;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.console.resources.DataCollectionDisplayInfo;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * This class controls all aspects of the application's execution
 */
public class NXMCApplication implements IApplication
{
	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.equinox.app.IApplication#start(org.eclipse.equinox.app.
	 * IApplicationContext)
	 */
	public Object start(IApplicationContext context) throws Exception
	{
		final Display display = PlatformUI.createDisplay();
		if (System.getProperty("sleak") != null) //$NON-NLS-1$
		{
			Sleak sleak = new Sleak();
			sleak.open();
		}

		try
		{
			ColorManager.create();
			StatusDisplayInfo.init(display);
			DataCollectionDisplayInfo.init();
			
			int returnCode = PlatformUI.createAndRunWorkbench(display, new NXMCWorkbenchAdvisor());
			if (returnCode == PlatformUI.RETURN_RESTART)
				return IApplication.EXIT_RESTART;
			else
				return IApplication.EXIT_OK;
		}
		finally
		{
			display.dispose();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.equinox.app.IApplication#stop()
	 */
	public void stop()
	{
		final IWorkbench workbench = PlatformUI.getWorkbench();
		if (workbench == null)
			return;
		final Display display = workbench.getDisplay();
		display.syncExec(new Runnable()
		{
			public void run()
			{
				if (!display.isDisposed())
					workbench.close();
			}
		});
	}
}
