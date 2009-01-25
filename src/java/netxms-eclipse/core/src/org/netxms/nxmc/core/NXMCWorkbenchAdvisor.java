package org.netxms.nxmc.core;

import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;

public class NXMCWorkbenchAdvisor extends WorkbenchAdvisor
{
	private static final String PERSPECTIVE_ID = "org.netxms.nxmc.core.perspective";

	public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		return new NXMCWorkbenchWindowAdvisor(configurer);
	}
	
	public String getInitialWindowPerspectiveId() 
	{
		return PERSPECTIVE_ID;
	}
}
