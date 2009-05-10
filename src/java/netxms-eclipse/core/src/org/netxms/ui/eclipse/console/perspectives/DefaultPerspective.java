/**
 * 
 */
package org.netxms.ui.eclipse.console.perspectives;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * @author Victor
 *
 */
public class DefaultPerspective implements IPerspectiveFactory
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IPerspectiveFactory#createInitialLayout(org.eclipse.ui.IPageLayout)
	 */
	@Override
	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
		layout.addView("org.netxms.ui.eclipse.objectbrowser.view.object_browser", IPageLayout.LEFT, 0, "");
		layout.addView("org.netxms.ui.eclipse.alarmviewer.view.alarm_browser", IPageLayout.RIGHT, 0.25f, "org.netxms.ui.eclipse.objectbrowser.view.object_browser");
		layout.addView("org.netxms.ui.eclipse.jobmonitor.view.job_monitor", IPageLayout.BOTTOM, 0.80f, "org.netxms.ui.eclipse.alarmviewer.view.alarm_browser");
	}
}
