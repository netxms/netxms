package org.netxms.ui.eclipse.console;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

public class Perspective implements IPerspectiveFactory
{
	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
	}
}
