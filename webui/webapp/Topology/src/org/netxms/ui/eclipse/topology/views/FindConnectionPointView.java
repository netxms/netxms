/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.topology.widgets.SearchResult;

/**
 * Search results for host connection searches
 *
 */
public class FindConnectionPointView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.HostSearchResults"; //$NON-NLS-1$
	private static final String TABLE_CONFIG_PREFIX = "HostSearchResults"; //$NON-NLS-1$
	
	private static SearchResult serachResultWidget;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
	   serachResultWidget = new SearchResult(this, parent, SWT.NONE, TABLE_CONFIG_PREFIX);
	   
		contributeToActionBars();
	}

	/**
	 * Contribute actions to action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		serachResultWidget.fillLocalPullDown(bars.getMenuManager());
		serachResultWidget.fillLocalToolBar(bars.getToolBarManager());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		serachResultWidget.setFocus();
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
		super.dispose();
	}
}
