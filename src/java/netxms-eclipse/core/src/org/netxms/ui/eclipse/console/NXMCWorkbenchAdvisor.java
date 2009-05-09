package org.netxms.ui.eclipse.console;

import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;

public class NXMCWorkbenchAdvisor extends WorkbenchAdvisor
{
	private static final String PERSPECTIVE_ID = "org.netxms.ui.eclipse.console.EmptyPerspective";

	public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		return new NXMCWorkbenchWindowAdvisor(configurer);
	}

	public String getInitialWindowPerspectiveId()
	{
		return PERSPECTIVE_ID;
	}
}
