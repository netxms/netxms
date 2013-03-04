/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;

/**
 * Show object's comments
 *
 */
public class Comments extends OverviewPageElement
{
	private static final long serialVersionUID = 1L;

	private Text comments;

	/**
	 * The constructor
	 * 
	 * @param parent
	 * @param object
	 */
	public Comments(Composite parent, GenericObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		comments = new Text(parent, SWT.MULTI | SWT.READ_ONLY | SWT.WRAP);
		comments.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, getDisplay()));
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
		return "Comments";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	void onObjectChange()
	{
		if (getObject() != null)
			comments.setText(getObject().getComments());
	}
}
