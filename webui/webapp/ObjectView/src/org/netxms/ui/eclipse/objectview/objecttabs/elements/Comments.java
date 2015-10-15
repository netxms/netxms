/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;

/**
 * Show object's comments
 *
 */
public class Comments extends OverviewPageElement
{
	private Text comments;

	/**
	 * The constructor
	 * 
	 * @param parent
	 * @param anchor
	 * @param objectTab
	 */
	public Comments(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
	{
		super(parent, anchor, objectTab);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		comments = new Text(parent, SWT.MULTI | SWT.READ_ONLY | SWT.WRAP);
		comments.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		if (getObject() != null)
			comments.setText(getObject().getComments());
		return comments;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().Comments_Title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		if (getObject() != null)
			comments.setText(getObject().getComments());
	}
}
