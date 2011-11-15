/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.console.perspectives;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * Default perspective
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
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.console.DefaultPerspective"); //$NON-NLS-1$
		layout.addPerspectiveShortcut("org.netxms.ui.eclipse.dashboard.DashboardPerspective"); //$NON-NLS-1$
		
		layout.addView("org.netxms.ui.eclipse.view.navigation.objectbrowser", IPageLayout.LEFT, 0, ""); //$NON-NLS-1$ //$NON-NLS-2$
		layout.addView("org.netxms.ui.eclipse.objectview.view.tabbed_object_view", IPageLayout.RIGHT, 0.25f, "org.netxms.ui.eclipse.view.navigation.objectbrowser"); //$NON-NLS-1$ //$NON-NLS-2$
	}
}
