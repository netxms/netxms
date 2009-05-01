package org.netxms.ui.eclipse.console.perspectives;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

public class EmptyPerspective implements IPerspectiveFactory
{
	public void createInitialLayout(IPageLayout layout)
	{
		layout.setEditorAreaVisible(false);
	}
}
