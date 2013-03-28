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

import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.widgets.CGroup;

/**
 * Abstract element of "Object Overview" tab
 *
 */
public abstract class OverviewPageElement extends CGroup
{
	private static final long serialVersionUID = 1L;

	private AbstractObject object;
	
	/**
	 * @param parent
	 * @param text
	 */
	public OverviewPageElement(Composite parent, AbstractObject object)
	{
		super(parent, "");
		setText(getTitle());
		this.object = object;
	}
	
	/**
	 * Get element's title
	 * 
	 * @return element's title
	 */
	abstract protected String getTitle();

	/**
	 * @return the object
	 */
	public AbstractObject getObject()
	{
		return object;
	}

	/**
	 * @param object the object to set
	 */
	public void setObject(AbstractObject object)
	{
		this.object = object;
		onObjectChange();
	}
	
	/**
	 * Handler for object change.
	 */
	abstract void onObjectChange();
	
	/**
	 * Check if element is applicable for given object. Default implementation
	 * always returns true.
	 * 
	 * @param object object to test
	 * @return true if element is applicable
	 */
	public boolean isApplicableForObject(AbstractObject object)
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		if (!isVisible())
			return new Point(0, 0);
		return super.computeSize(wHint, hHint, changed);
	}
}
