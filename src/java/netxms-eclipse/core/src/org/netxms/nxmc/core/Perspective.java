package org.netxms.nxmc.core;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

public class Perspective implements IPerspectiveFactory
{
	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
	}
}
