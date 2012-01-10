package org.netxms.webui.core;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * Configures the perspective layout. This class is contributed through the
 * plugin.xml.
 */
public class Perspective implements IPerspectiveFactory
{

	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.console.DefaultPerspective"); //$NON-NLS-1$
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.dashboard.DashboardPerspective"); //$NON-NLS-1$
		
		layout.addView("org.netxms.ui.eclipse.view.navigation.objectbrowser", IPageLayout.LEFT, 0, ""); //$NON-NLS-1$ //$NON-NLS-2$
		layout.addView("org.netxms.ui.eclipse.objectview.view.tabbed_object_view", IPageLayout.RIGHT, 0.25f, "org.netxms.ui.eclipse.view.navigation.objectbrowser"); //$NON-NLS-1$ //$NON-NLS-2$
	}
}
