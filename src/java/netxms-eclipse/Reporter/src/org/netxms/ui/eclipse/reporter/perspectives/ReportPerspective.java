package org.netxms.ui.eclipse.reporter.perspectives;

import org.eclipse.ui.IFolderLayout;
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

      final IFolderLayout navigationFolder = layout.createFolder("org.netxms.ui.eclipse.folders.navigation", IPageLayout.LEFT, 0.20f, ""); //$NON-NLS-1$ //$NON-NLS-2$
      navigationFolder.addView(ReportNavigator.ID);

      final IFolderLayout mainFolder = layout.createFolder("org.netxms.ui.eclipse.folders.main", IPageLayout.RIGHT, 0.20f, "org.netxms.ui.eclipse.folders.navigation"); //$NON-NLS-1$ //$NON-NLS-2$
      mainFolder.addView(ReportView.ID);
      mainFolder.addPlaceholder("*"); //$NON-NLS-1$
	}
}
