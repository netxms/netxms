package org.netxms.ui.eclipse.reporter.perspectives;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;
import org.netxms.ui.eclipse.reporter.views.ReportNavigator;
import org.netxms.ui.eclipse.reporter.views.ReportView;

public class ReportPerspective implements IPerspectiveFactory
{
	@Override
	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.console.DefaultPerspective"); //$NON-NLS-1$
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.dashboard.DashboardPerspective"); //$NON-NLS-1$
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.reporter.ReportPerspective"); //$NON-NLS-1$

		layout.addView(ReportNavigator.ID, IPageLayout.LEFT, 0, null);
		layout.addView(ReportView.ID, IPageLayout.RIGHT, 0.25f, ReportNavigator.ID);
	}

}
